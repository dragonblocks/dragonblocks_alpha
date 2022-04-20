#include <asprintf/asprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/client_config.h"
#include "client/client_inventory.h"
#include "client/client_item.h"
#include "client/gl_debug.h"
#include "client/frustum.h"
#include "client/light.h"
#include "client/shader.h"

static GLuint _3d_shader_prog;
static GLint _3d_loc_VP;
static GLint _3d_loc_depthOffset;
static ModelShader _3d_model_shader;
static LightShader _3d_light_shader;

bool client_inventory_init()
{
	char *_3d_shader_defs;
	asprintf(&_3d_shader_defs, "#define VIEW_DISTANCE %lf\n", client_config.view_distance);

	if (!shader_program_create(RESSOURCE_PATH "shaders/3d/item", &_3d_shader_prog, _3d_shader_defs)) {
		fprintf(stderr, "[error] failed to create 3D item shader program\n");
		return false;
	}

	free(_3d_shader_defs);

	_3d_loc_VP = glGetUniformLocation(_3d_shader_prog, "VP");
	_3d_loc_depthOffset = glGetUniformLocation(_3d_shader_prog, "depthOffset");

	_3d_model_shader.prog = _3d_shader_prog;
	_3d_model_shader.loc_transform = glGetUniformLocation(_3d_shader_prog, "model"); GL_DEBUG

	_3d_light_shader.prog = _3d_shader_prog;
	light_shader_locate(&_3d_light_shader);

	client_inventory_depth_offset(0.0f);

	return true;
}

void client_inventory_deinit()
{
	glDeleteProgram(_3d_shader_prog); GL_DEBUG
}

void client_inventory_update()
{
	glProgramUniformMatrix4fv(_3d_shader_prog, _3d_loc_VP, 1, GL_FALSE, frustum[0]); GL_DEBUG
	light_shader_update(&_3d_light_shader);
}

void client_inventory_depth_offset(f32 offset)
{
	glProgramUniform1f(_3d_shader_prog, _3d_loc_depthOffset, offset);
}

static void wield_init(ModelNode *hand)
{
	if (hand)
		model_node_add_mesh(hand, &(ModelMesh) {
			.mesh = NULL,
			.textures = NULL,
			.num_textures = 0,
			.shader = &_3d_model_shader,
		});
}

static void wield_update(ModelNode *hand, ModelNode *arm, ItemType item)
{
	Mesh *mesh = client_item_mesh(item);

	if (hand)
		((ModelMesh *) hand->meshes.ptr)[0].mesh = mesh;

	if (arm) {
		arm->rot.x = mesh ? -M_PI / 8.0 : 0.0;
		model_node_transform(arm);
	}
}

void client_inventory_init_player(ClientEntity *entity)
{
	ClientPlayerData *data = entity->extra;

	pthread_mutex_init(&data->mtx_inv, NULL);

	item_stack_initialize(&data->inventory.left);
	item_stack_initialize(&data->inventory.right);

	wield_init(data->bones.hand_left);
	wield_init(data->bones.hand_right);
}

void client_inventory_deinit_player(ClientEntity *entity)
{
	ClientPlayerData *data = entity->extra;

	pthread_mutex_destroy(&data->mtx_inv);

	item_stack_destroy(&data->inventory.left);
	item_stack_destroy(&data->inventory.right);
}

void client_inventory_update_player(__attribute__((unused)) void *peer, ToClientPlayerInventory *pkt)
{
	ClientEntity *entity = client_player_entity(pkt->id);
	if (!entity)
		return;

	ClientPlayerData *data = entity->extra;
	pthread_mutex_lock(&data->mtx_inv);

	item_stack_deserialize(&data->inventory.left, &pkt->left);
	item_stack_deserialize(&data->inventory.right, &pkt->right);

	wield_update(data->bones.hand_left,  data->bones.arm_left,  data->inventory.left.type);
	wield_update(data->bones.hand_right, data->bones.arm_right, data->inventory.right.type);

	pthread_mutex_unlock(&data->mtx_inv);
	refcount_drp(&entity->rc);
}
