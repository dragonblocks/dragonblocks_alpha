#include <linmath.h>
#include <stdio.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_item.h"
#include "client/client_node.h"
#include "client/client_player.h"
#include "client/cube.h"
#include "client/debug_menu.h"
#include "client/frustum.h"
#include "client/opengl.h"
#include "client/gui.h"
#include "client/interact.h"
#include "client/mesh.h"
#include "client/raycast.h"
#include "client/shader.h"

struct InteractPointed interact_pointed;

static GLuint shader_prog;
static GLint loc_MVP;
static GLint loc_color;
static mat4x4 model;

typedef struct {
	v3f32 position;
} __attribute__((packed)) SelectionVertex;
static Mesh selection_mesh = {
	.layout = &(VertexLayout) {
		.attributes = (VertexAttribute[]) {
			{GL_FLOAT, 3, sizeof(v3f32)},
		},
		.count = 1,
		.size = sizeof(SelectionVertex),
	},
	.vao = 0,
	.vbo = 0,
	.data = NULL,
	.count = 36,
	.free_data = false,
};

void interact_init()
{
	shader_prog = shader_program_create(ASSET_PATH "shaders/3d/selection", NULL);
	loc_MVP = glGetUniformLocation(shader_prog, "MVP"); GL_DEBUG
	loc_color = glGetUniformLocation(shader_prog, "color"); GL_DEBUG

	SelectionVertex vertices[6][6];
	for (int f = 0; f < 6; f++)
		for (int v = 0; v < 6; v++)
			vertices[f][v].position = v3f32_scale(cube_vertices[f][v].position, 1.1f);

	selection_mesh.data = vertices;
	mesh_upload(&selection_mesh);

	gui_add(NULL, (GUIElementDef) {
		.pos = {0.5f, 0.5f},
		.z_index = 0.0f,
		.offset = {0, 0},
		.margin = {0, 0},
		.align = {0.5f, 0.5f},
		.scale = {1.0f, 1.0f},
		.scale_type = SCALE_IMAGE,
		.affect_parent_scale = false,
		.text = NULL,
		.image = texture_load(ASSET_PATH "textures/crosshair.png", false),
		.text_color = {0.0f, 0.0f, 0.0f, 0.0f},
		.bg_color = {0.0f, 0.0f, 0.0f, 0.0f},
	});
}

void interact_deinit()
{
	glDeleteProgram(shader_prog); GL_DEBUG
	mesh_destroy(&selection_mesh);
}

#include "client/client_terrain.h"

void interact_tick()
{
	bool old_exists = interact_pointed.exists;
	v3s32 old_pointed = interact_pointed.pos;
	if ((interact_pointed.exists = raycast(
				(v3f64) {camera.eye  [0], camera.eye  [1], camera.eye  [2]},
				(v3f64) {camera.front[0], camera.front[1], camera.front[2]},
				5, &interact_pointed.pos, &interact_pointed.node))
			&& !v3s32_equals(interact_pointed.pos, old_pointed)) {
		mat4x4_translate(model,
			interact_pointed.pos.x, interact_pointed.pos.y, interact_pointed.pos.z);
		v3f32 *color = &client_node_def[interact_pointed.node].selection_color;
		glProgramUniform3f(shader_prog, loc_color, color->x, color->y, color->z); GL_DEBUG
		debug_menu_changed(ENTRY_POINTED);
	}

	if (old_exists && !interact_pointed.exists)
		debug_menu_changed(ENTRY_POINTED);
}

void interact_render()
{
	if (!interact_pointed.exists)
		return;

	mat4x4 mvp;
	mat4x4_mul(mvp, frustum, model);

	glUseProgram(shader_prog); GL_DEBUG
	glUniformMatrix4fv(loc_MVP, 1, GL_FALSE, mvp[0]); GL_DEBUG
	mesh_render(&selection_mesh);
}

void interact_use(bool right)
{
	ClientEntity *entity = client_player_entity_local();
	if (!entity)
		return;

	ClientPlayerData *data = entity->extra;
	pthread_mutex_lock(&data->mtx_inv);

	ItemStack *stack = &data->inventory.hands[right];
	if (client_item_def[stack->type].use && client_item_def[stack->type].use(stack))
		dragonnet_peer_send_ToServerInteract(client, &(ToServerInteract) {
			.right = right,
			.pointed = interact_pointed.exists,
			.pos = interact_pointed.pos,
		});

	pthread_mutex_unlock(&data->mtx_inv);
	refcount_drp(&entity->rc);
}
