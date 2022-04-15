#include <asprintf/asprintf.h>
#include <linmath.h/linmath.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/camera.h"
#include "client/client_config.h"
#include "client/client_node.h"
#include "client/client_terrain.h"
#include "client/cube.h"
#include "client/frustum.h"
#include "client/shader.h"
#include "client/terrain_gfx.h"
#include "day.h"

typedef struct {
	bool visible;
	bool transparent;
	ModelBatch *batch;
	TerrainChunk *chunk;
	v3s32 chunkp;
	TerrainChunk *nbrs[6];
	bool tried_nbrs[6];
	bool cull_edges;
} ChunkRenderData;

static VertexLayout terrain_vertex_layout = {
	.attributes = (VertexAttribute []) {
		{GL_FLOAT, 3, sizeof(v3f32)}, // position
		{GL_FLOAT, 3, sizeof(v3f32)}, // normal
		{GL_FLOAT, 2, sizeof(v2f32)}, // textureCoordinates
		{GL_FLOAT, 1, sizeof(f32  )}, // textureIndex
		{GL_FLOAT, 3, sizeof(v3f32)}, // color
	},
	.count = 5,
	.size = sizeof(TerrainVertex),
};

static v3s32 face_dir[6] = {
	{+0, +0, -1},
	{+0, +0, +1},
	{-1, +0, +0},
	{+1, +0, +0},
	{+0, -1, +0},
	{+0, +1, +0},
};

static v3f32 center_offset = {
	CHUNK_SIZE * 0.5f + 0.5f,
	CHUNK_SIZE * 0.5f + 0.5f,
	CHUNK_SIZE * 0.5f + 0.5f,
};

static GLuint shader_prog;
static GLint loc_model;
static GLint loc_VP;
static GLint loc_daylight;
static GLint loc_fogColor;
static GLint loc_ambientLight;
static GLint loc_lightDir;
static GLint loc_cameraPos;

static ModelShader model_shader;

static inline bool cull_face(NodeType self, NodeType nbr)
{
	switch (client_node_definitions[self].visibility) {
		case VISIBILITY_CLIP:
			return false;

		case VISIBILITY_BLEND:
			return self == nbr;

		case VISIBILITY_SOLID:
			return nbr == NODE_UNLOADED
				|| client_node_definitions[nbr].visibility == VISIBILITY_SOLID;

		default: // impossible
			break;
	}

	// impossible
	return false;
}

static inline void render_node(ChunkRenderData *data, v3s32 offset)
{
	NodeArgsRender args;

	args.node = &data->chunk->data[offset.x][offset.y][offset.z];

	ClientNodeDefinition *def = &client_node_definitions[args.node->type];
	if (def->visibility == VISIBILITY_NONE)
		return;

	v3f32 vertex_offset = v3f32_sub(v3s32_to_f32(offset), center_offset);
	if (def->render)
		args.pos = v3s32_add(offset, data->chunkp);

	for (args.f = 0; args.f < 6; args.f++) {
		v3s32 nbr_offset = v3s32_add(offset, face_dir[args.f]);

		TerrainChunk *nbr_chunk;

		if (nbr_offset.z >= 0 && nbr_offset.z < CHUNK_SIZE &&
			nbr_offset.x >= 0 && nbr_offset.x < CHUNK_SIZE &&
			nbr_offset.y >= 0 && nbr_offset.y < CHUNK_SIZE) {
			nbr_chunk = data->chunk;
		} else if (!(nbr_chunk = data->nbrs[args.f]) && !data->tried_nbrs[args.f]) {
			nbr_chunk = data->nbrs[args.f] = terrain_get_chunk(client_terrain,
				v3s32_add(data->chunk->pos, face_dir[args.f]), false);
			data->tried_nbrs[args.f] = true;
		}

		NodeType nbr_node = NODE_UNLOADED;
		if (nbr_chunk)
			nbr_node = nbr_chunk->data
				[(nbr_offset.x + CHUNK_SIZE) % CHUNK_SIZE]
				[(nbr_offset.y + CHUNK_SIZE) % CHUNK_SIZE]
				[(nbr_offset.z + CHUNK_SIZE) % CHUNK_SIZE].type;

		if (cull_face(args.node->type, nbr_node)) {
			// exception to culling rules: don't cull solid edge nodes, unless cull_edges
			if (data->cull_edges || nbr_chunk == data->chunk || def->visibility != VISIBILITY_SOLID)
				continue;
		} else {
			data->visible = true;
		}

		if (def->visibility == VISIBILITY_BLEND)
			data->transparent = true;

		for (args.v = 0; args.v < 6; args.v++) {
			args.vertex.cube = cube_vertices[args.f][args.v];
			args.vertex.cube.position = v3f32_add(args.vertex.cube.position, vertex_offset);
			args.vertex.color = (v3f32) {1.0f, 1.0f, 1.0f};

			if (def->render)
				def->render(&args);

			model_batch_add_vertex(data->batch, def->tiles.textures[args.f]->txo, &args.vertex);
		}
	}
}

static void animate_chunk_model(Model *model, f64 dtime)
{
	if ((model->root->scale.x += dtime * 2.0f) > 1.0f) {
		model->root->scale.x = 1.0f;
		client_terrain_meshgen_task(model->extra);
	}

	model->root->scale.z
		= model->root->scale.y
		= model->root->scale.x;

	model_node_transform(model->root);
}

static Model *create_chunk_model(TerrainChunk *chunk, bool animate)
{
	ChunkRenderData data = {
		.visible = false,
		.transparent = false,
		.batch = model_batch_create(&model_shader, &terrain_vertex_layout, offsetof(TerrainVertex, textureIndex)),
		.chunk = chunk,
		.chunkp = v3s32_scale(chunk->pos, CHUNK_SIZE),
		.nbrs = {NULL},
		.tried_nbrs = {false},
		.cull_edges = !animate,
	};

	CHUNK_ITERATE
		render_node(&data, (v3s32) {x, y, z});

	if (!data.batch->textures.siz) {
		model_batch_free(data.batch);
		return NULL;
	}

	Model *model = model_create();
	model->extra = chunk;
	model->box = (aabb3f32) {
		v3f32_sub((v3f32) {-1.0f, -1.0f, -1.0f}, center_offset),
		v3f32_add((v3f32) {+1.0f, +1.0f, +1.0f}, center_offset)};
	model->callbacks.step = animate ? &animate_chunk_model : NULL;
	model->callbacks.delete = &model_free_meshes;
	model->flags.frustum_culling = 1;
	model->flags.transparent = data.transparent;

	model->root->visible = data.visible;
	model->root->pos = v3f32_add(v3s32_to_f32(data.chunkp), center_offset);
	model->root->scale = (v3f32) {0.0f, 0.0f, 0.0f};

	if (data.visible)
		model_node_add_batch(model->root, data.batch);
	else
		model_batch_free(data.batch);

	return model;
}

bool terrain_gfx_init()
{
	GLint texture_units;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texture_units);

	char *shader_defs;
	asprintf(&shader_defs,
		"#define MAX_TEXTURE_UNITS %d\n"
		"#define VIEW_DISTANCE %lf\n",
		texture_units,
		client_config.view_distance
	);

	if (!shader_program_create(RESSOURCE_PATH "shaders/terrain", &shader_prog, shader_defs)) {
		fprintf(stderr, "[error] failed to create terrain shader program\n");
		return false;
	}

	free(shader_defs);

	loc_model = glGetUniformLocation(shader_prog, "model");
	loc_VP = glGetUniformLocation(shader_prog, "VP");
	loc_daylight = glGetUniformLocation(shader_prog, "daylight");
	loc_fogColor = glGetUniformLocation(shader_prog, "fogColor");
	loc_ambientLight = glGetUniformLocation(shader_prog, "ambientLight");
	loc_lightDir = glGetUniformLocation(shader_prog, "lightDir");
	loc_cameraPos = glGetUniformLocation(shader_prog, "cameraPos");

	GLint texture_indices[texture_units];
	for (GLint i = 0; i < texture_units; i++)
		texture_indices[i] = i;

	glProgramUniform1iv(shader_prog, glGetUniformLocation(shader_prog, "textures"), texture_units, texture_indices);

	model_shader.prog = shader_prog;
	model_shader.loc_transform = loc_model;

	return true;
}

void terrain_gfx_deinit()
{
	glDeleteProgram(shader_prog);
}

void terrain_gfx_update()
{
	vec4 base_sunlight_dir = {0.0f, 0.0f, -1.0f, 1.0f};
	vec4 sunlight_dir;
	mat4x4 sunlight_mat;
	mat4x4_identity(sunlight_mat);

	mat4x4_rotate(sunlight_mat, sunlight_mat, 1.0f, 0.0f, 0.0f, get_sun_angle() + M_PI / 2.0f);
	mat4x4_mul_vec4(sunlight_dir, sunlight_mat, base_sunlight_dir);

	f32 daylight = get_daylight();
	f32 ambient_light = f32_mix(0.3f, 0.7f, daylight);
	v3f32 fog_color = v3f32_mix((v3f32) {0x03, 0x0A, 0x1A}, (v3f32) {0x87, 0xCE, 0xEB}, daylight);

	glProgramUniformMatrix4fv(shader_prog, loc_VP, 1, GL_FALSE, frustum[0]);
	glProgramUniform3f(shader_prog, loc_lightDir, sunlight_dir[0], sunlight_dir[1], sunlight_dir[2]);
	glProgramUniform3f(shader_prog, loc_cameraPos, camera.eye[0], camera.eye[1], camera.eye[2]);
	glProgramUniform1f(shader_prog, loc_daylight, daylight);
	glProgramUniform3f(shader_prog, loc_fogColor, fog_color.x / 0xFF * ambient_light, fog_color.y / 0xFF * ambient_light, fog_color.z / 0xFF * ambient_light);
	glProgramUniform1f(shader_prog, loc_ambientLight, ambient_light);
}

void terrain_gfx_make_chunk_model(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;

	bool animate = true;

	pthread_mutex_lock(&chunk->mtx);
	if (meta->model && meta->model->root->scale.x == 1.0f)
		animate = false;
	pthread_mutex_unlock(&chunk->mtx);

	Model *model = create_chunk_model(chunk, animate);

	pthread_mutex_lock(&chunk->mtx);

	if (meta->model) {
		if (model) {
			model->callbacks.step = meta->model->callbacks.step;
			model->root->scale = meta->model->root->scale;
			model_node_transform(model->root);
		}

		meta->model->flags.delete = 1;
	}

	meta->model = model;

	if (model)
		model_scene_add(model);

	pthread_mutex_unlock(&chunk->mtx);
}
