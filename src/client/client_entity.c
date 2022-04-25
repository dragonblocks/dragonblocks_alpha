#include <asprintf.h>
#include <dragonstd/map.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/cube.h"
#include "client/client_config.h"
#include "client/client_entity.h"
#include "client/client_player.h"
#include "client/frustum.h"
#include "client/gl_debug.h"
#include "client/light.h"
#include "client/shader.h"
#include "client/window.h"

ClientEntityType client_entity_types[COUNT_ENTITY];

ModelShader client_entity_shader;
typedef struct {
	v3f32 position;
	v3f32 normal;
} __attribute__((packed)) EntityVertex;
Mesh client_entity_cube = {
	.layout = &(VertexLayout) {
		.attributes = (VertexAttribute[]) {
			{GL_FLOAT, 3, sizeof(v3f32)}, // position
			{GL_FLOAT, 3, sizeof(v3f32)}, // normal
		},
		.count = 2,
		.size = sizeof(EntityVertex),
	},
	.vao = 0,
	.vbo = 0,
	.data = NULL,
	.count = 36,
	.free_data = false,
};

static GLuint shader_prog;
static GLint loc_VP;
static GLint loc_depthOffset;
static LightShader light_shader;
static Map entities;
static List nametagged;
static pthread_mutex_t mtx_nametagged;

// any thread
// called when adding, getting or removing an entity from the map
static int cmp_entity(const Refcount *entity, const u64 *id)
{
	return u64_cmp(&((ClientEntity *) entity->obj)->data.id, id);
}

// recv thread
// called when server sent removal of entity
static void entity_drop(ClientEntity *entity)
{
	if (entity->type->remove)
		entity->type->remove(entity);

	if (entity->nametag) {
		pthread_mutex_lock(&mtx_nametagged);
		list_del(&nametagged, &entity->rc, &cmp_ref, &refcount_drp, NULL, NULL);
		pthread_mutex_unlock(&mtx_nametagged);

		entity->nametag->visible = false;
	}

	refcount_drp(&entity->rc);
}

// any thread
// called when all refs have been dropped
static void entity_delete(ClientEntity *entity)
{
	if (entity->type->free)
		entity->type->free(entity);

	refcount_dst(&entity->rc);

	if (entity->data.nametag)
		free(entity->data.nametag);

	pthread_rwlock_init(&entity->lock_pos_rot, NULL);
	pthread_rwlock_init(&entity->lock_nametag, NULL);
	pthread_rwlock_init(&entity->lock_box_off, NULL);

	free(entity);
}

static void update_nametag(ClientEntity *entity)
{
	if (entity->nametag) {
		gui_text(entity->nametag, entity->data.nametag);

		if (!entity->data.nametag)
			entity->nametag->visible = false;
	} else if (entity->data.nametag) {
		entity->nametag = gui_add(NULL, (GUIElementDef) {
			.pos = {-1.0f, -1.0f},
			.z_index = 0.1f,
			.offset = {0, 0},
			.margin = {4, 4},
			.align = {0.5f, 0.5f},
			.scale = {1.0f, 1.0f},
			.scale_type = SCALE_TEXT,
			.affect_parent_scale = false,
			.text = entity->data.nametag,
			.image = NULL,
			.text_color = (v4f32) {1.0f, 1.0f, 1.0f, 1.0f},
			.bg_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.5f},
		});

		pthread_mutex_lock(&mtx_nametagged);
		list_apd(&nametagged, refcount_inc(&entity->rc));
		pthread_mutex_unlock(&mtx_nametagged);
	}
}

static void update_nametag_pos(ClientEntity *entity)
{
	if (!entity->data.nametag)
		return;

	pthread_rwlock_rdlock(&entity->lock_pos_rot);
	pthread_rwlock_rdlock(&entity->lock_box_off);

	mat4x4 mvp;
	if (entity->nametag_offset)
		mat4x4_mul(mvp, frustum, *entity->nametag_offset);
	else
		mat4x4_dup(mvp, frustum);

	vec4 dst, src = {0.0f, 0.0f, 0.0f, 1.0f};
	mat4x4_mul_vec4(dst, mvp, src);

	dst[0] /= dst[3];
	dst[1] /= dst[3];
	dst[2] /= dst[3];

	if ((entity->nametag->visible = dst[2] >= -1.0f && dst[2] <= 1.0f)) {
		entity->nametag->def.pos = (v2f32) {dst[0] * 0.5f + 0.5f, 1.0f - (dst[1] * 0.5f + 0.5f)};
		gui_transform(entity->nametag);
	}

	pthread_rwlock_unlock(&entity->lock_box_off);
	pthread_rwlock_unlock(&entity->lock_pos_rot);
}

// main thread
// called on startup
void client_entity_init()
{
	map_ini(&entities);
	list_ini(&nametagged);
	pthread_mutex_init(&mtx_nametagged, NULL);
}

// main thead
// called on shutdown
void client_entity_deinit()
{
	// forget all entities
	map_cnl(&entities, &refcount_drp, NULL, NULL, 0);
	list_clr(&nametagged, &refcount_drp, NULL, NULL);
	pthread_mutex_destroy(&mtx_nametagged);
}

void client_entity_gfx_init()
{
	char *shader_def;
	asprintf(&shader_def, "#define VIEW_DISTANCE %lf\n", client_config.view_distance);
	shader_prog = shader_program_create(ASSET_PATH "shaders/3d/entity", shader_def);
	free(shader_def);

	loc_VP = glGetUniformLocation(shader_prog, "VP"); GL_DEBUG
	loc_depthOffset = glGetUniformLocation(shader_prog, "depthOffset"); GL_DEBUG

	EntityVertex vertices[6][6];
	for (int f = 0; f < 6; f++) {
		for (int v = 0; v < 6; v++) {
			vertices[f][v].position = cube_vertices[f][v].position;
			vertices[f][v].normal = cube_vertices[f][v].normal;
		}
	}

	client_entity_cube.data = vertices;
	mesh_upload(&client_entity_cube);

	client_entity_shader.prog = shader_prog;
	client_entity_shader.loc_transform = glGetUniformLocation(shader_prog, "model"); GL_DEBUG

	light_shader.prog = shader_prog;
	light_shader_locate(&light_shader);

	client_entity_depth_offset(0.0f);
}

void client_entity_gfx_deinit()
{
	glDeleteProgram(shader_prog); GL_DEBUG
	mesh_destroy(&client_entity_cube);
}

void client_entity_gfx_update()
{
	glProgramUniformMatrix4fv(shader_prog, loc_VP, 1, GL_FALSE, frustum[0]); GL_DEBUG
	light_shader_update(&light_shader);

	pthread_mutex_lock(&mtx_nametagged);
	list_itr(&nametagged, &update_nametag_pos, NULL, &refcount_obj);
	pthread_mutex_unlock(&mtx_nametagged);
}

void client_entity_depth_offset(f32 offset)
{
	glProgramUniform1f(shader_prog, loc_depthOffset, offset);
}

ClientEntity *client_entity_grab(u64 id)
{
	return id ? map_get(&entities, &id, &cmp_entity, &refcount_grb) : NULL;
}

void client_entity_transform(ClientEntity *entity)
{
	if (!entity->model)
		return;

	entity->model->root->pos = v3f64_to_f32(entity->data.pos); // ToDo: the render pipeline needs to be updated to handle 64-bit positions
	entity->model->root->rot = entity->data.rot;

	if (entity->type->transform)
		entity->type->transform(entity);

	model_node_transform(entity->model->root);
}

void client_entity_add(__attribute__((unused)) void *peer, ToClientEntityAdd *pkt)
{
	if (pkt->type >= COUNT_ENTITY)
		return;

	ClientEntity *entity = malloc(sizeof *entity);

	entity->data = pkt->data;
	entity->type = &client_entity_types[pkt->type];
	refcount_ini(&entity->rc, entity, &entity_delete);

	pkt->data.nametag = NULL;

	entity->model = NULL;
	entity->nametag = NULL;

	if (entity->type->add)
		entity->type->add(entity);

	update_nametag(entity);

	pthread_rwlock_init(&entity->lock_pos_rot, NULL);
	pthread_rwlock_init(&entity->lock_nametag, NULL);
	pthread_rwlock_init(&entity->lock_box_off, NULL);

	if (!map_add(&entities, &entity->data.id, &entity->rc, &cmp_entity, &refcount_inc))
		fprintf(stderr, "[warning] failed to add entity %" PRIu64 "\n", entity->data.id);

	refcount_drp(&entity->rc);
}

void client_entity_remove(__attribute__((unused)) void *peer, ToClientEntityRemove *pkt)
{
	map_del(&entities, &pkt->id, &cmp_entity, &entity_drop, NULL, &refcount_obj);
}

void client_entity_update_pos_rot(__attribute__((unused)) void *peer, ToClientEntityUpdatePosRot *pkt)
{
	ClientEntity *entity = client_entity_grab(pkt->id);

	if (!entity)
		return;

	pthread_rwlock_wrlock(&entity->lock_pos_rot);

	entity->data.pos = pkt->pos;
	entity->data.rot = pkt->rot;

	if (entity->type->update_pos_rot)
		entity->type->update_pos_rot(entity);

	client_entity_transform(entity);

	pthread_rwlock_unlock(&entity->lock_pos_rot);

	refcount_drp(&entity->rc);
}

void client_entity_update_nametag(__attribute__((unused)) void *peer, ToClientEntityUpdateNametag *pkt)
{
	ClientEntity *entity = client_entity_grab(pkt->id);

	if (!entity)
		return;

	pthread_rwlock_wrlock(&entity->lock_nametag);

	if (entity->data.nametag)
		free(entity->data.nametag);

	entity->data.nametag = pkt->nametag;
	pkt->nametag = NULL;

	if (entity->type->update_nametag)
		entity->type->update_nametag(entity);

	update_nametag(entity);
	pthread_rwlock_unlock(&entity->lock_nametag);

	refcount_drp(&entity->rc);
}
