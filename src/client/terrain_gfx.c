#include <asprintf/asprintf.h>
#include <linmath.h/linmath.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/client_config.h"
#include "client/client_node.h"
#include "client/client_terrain.h"
#include "client/cube.h"
#include "client/facedir.h"
#include "client/frustum.h"
#include "client/gl_debug.h"
#include "client/light.h"
#include "client/shader.h"
#include "client/terrain_gfx.h"

typedef struct {
	bool visible;
	bool transparent;
	ModelBatch *batch;
	ModelBatch *batch_transparent;
	TerrainChunk *chunk;
	v3s32 chunkp;
	TerrainChunk *nbrs[6];
	bool *tried_nbrs;
	bool show_edges;
	bool shown_edges;
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

static v3f32 center_offset = {
	CHUNK_SIZE * 0.5f + 0.5f,
	CHUNK_SIZE * 0.5f + 0.5f,
	CHUNK_SIZE * 0.5f + 0.5f,
};

static GLuint shader_prog;
static GLint loc_VP;

static LightShader light_shader;
static ModelShader model_shader;

static inline bool show_face(NodeType self, NodeType nbr)
{
	switch (client_node_defs[self].visibility) {
		case VISIBILITY_CLIP:
			return true;

		case VISIBILITY_BLEND:
			return nbr != self;

		case VISIBILITY_SOLID:
			return nbr != NODE_UNLOADED && client_node_defs[nbr].visibility != VISIBILITY_SOLID;

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

	ClientNodeDef *def = &client_node_defs[args.node->type];
	if (def->visibility == VISIBILITY_NONE)
		return;

	v3f32 vertex_offset = v3f32_sub(v3s32_to_f32(offset), center_offset);
	if (def->render)
		args.pos = v3s32_add(offset, data->chunkp);

	for (args.f = 0; args.f < 6; args.f++) {
		v3s32 nbr_offset = v3s32_add(offset, facedir[args.f]);

		TerrainChunk *nbr_chunk;

		if (nbr_offset.z >= 0 && nbr_offset.z < CHUNK_SIZE &&
			nbr_offset.x >= 0 && nbr_offset.x < CHUNK_SIZE &&
			nbr_offset.y >= 0 && nbr_offset.y < CHUNK_SIZE) {
			nbr_chunk = data->chunk;
		} else if (!(nbr_chunk = data->nbrs[args.f]) && !data->tried_nbrs[args.f]) {
			nbr_chunk = data->nbrs[args.f] = terrain_get_chunk(client_terrain,
				v3s32_add(data->chunk->pos, facedir[args.f]), false);
			data->tried_nbrs[args.f] = true;
		}

		NodeType nbr_node = NODE_UNLOADED;
		if (nbr_chunk)
			nbr_node = nbr_chunk->data
				[(nbr_offset.x + CHUNK_SIZE) % CHUNK_SIZE]
				[(nbr_offset.y + CHUNK_SIZE) % CHUNK_SIZE]
				[(nbr_offset.z + CHUNK_SIZE) % CHUNK_SIZE].type;

		bool show = show_face(args.node->type, nbr_node);

		if (show)
			data->visible = true;
		else if (data->show_edges && nbr_chunk != data->chunk && def->visibility == VISIBILITY_SOLID)
			data->shown_edges = show = true;

		if (!show)
			continue;

		ModelBatch *batch = data->batch;

		if (def->visibility == VISIBILITY_BLEND) {
			data->transparent = true;
			batch = data->batch_transparent;
		}

		for (args.v = 0; args.v < 6; args.v++) {
			args.vertex.cube = cube_vertices[args.f][args.v];
			args.vertex.cube.position = v3f32_add(args.vertex.cube.position, vertex_offset);
			args.vertex.color = (v3f32) {1.0f, 1.0f, 1.0f};

			if (def->render)
				def->render(&args);

			model_batch_add_vertex(batch, def->tiles.textures[args.f]->txo, &args.vertex);
		}
	}
}

static void animate_chunk_model(Model *model, f64 dtime)
{
	bool finished = (model->root->scale.x += dtime * 2.0f) > 1.0f;
	if (finished)
		model->root->scale.x = 1.0f;

	model->root->scale.z
		= model->root->scale.y
		= model->root->scale.x;

	model_node_transform(model->root);

	if (finished) {
		model->callbacks.step = NULL;

		if (model->extra) {
			TerrainChunk *chunk = model->extra;

			pthread_mutex_lock(&chunk->mtx);
			client_terrain_meshgen_task(chunk, false);
			pthread_mutex_unlock(&chunk->mtx);
		}
	}
}

static Model *create_chunk_model(TerrainChunk *chunk, bool animate, bool *depends)
{
	ChunkRenderData data = {
		.visible = false,
		.transparent = false,
		.batch = model_batch_create(&model_shader, &terrain_vertex_layout, offsetof(TerrainVertex, textureIndex)),
		.batch_transparent = model_batch_create(&model_shader, &terrain_vertex_layout, offsetof(TerrainVertex, textureIndex)),
		.chunk = chunk,
		.chunkp = v3s32_scale(chunk->pos, CHUNK_SIZE),
		.nbrs = {NULL},
		.tried_nbrs = depends,
		.show_edges = animate,
		.shown_edges = false,
	};

	CHUNK_ITERATE
		render_node(&data, (v3s32) {x, y, z});

	if (!data.visible || (!data.batch->textures.siz && !data.batch_transparent->textures.siz)) {
		model_batch_free(data.batch);
		model_batch_free(data.batch_transparent);
		return NULL;
	}

	Model *model = model_create();
	if (data.shown_edges)
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

	if (data.visible) {
		model_node_add_batch(model->root, data.batch);
		model_node_add_batch(model->root, data.batch_transparent);
	} else {
		model_batch_free(data.batch);
		model_batch_free(data.batch_transparent);
	}

	for (int i = 0; i < 6; i++)
		if (data.nbrs[i])
			data.tried_nbrs[i] = true;

	return model;
}

bool terrain_gfx_init()
{
	GLint texture_units;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &texture_units); GL_DEBUG

	char *shader_defs;
	asprintf(&shader_defs,
		"#define MAX_TEXTURE_UNITS %d\n"
		"#define VIEW_DISTANCE %lf\n",
		texture_units,
		client_config.view_distance
	);

	if (!shader_program_create(RESSOURCE_PATH "shaders/3d/terrain", &shader_prog, shader_defs)) {
		fprintf(stderr, "[error] failed to create terrain shader program\n");
		return false;
	}

	free(shader_defs);

	loc_VP = glGetUniformLocation(shader_prog, "VP"); GL_DEBUG

	GLint texture_indices[texture_units];
	for (GLint i = 0; i < texture_units; i++)
		texture_indices[i] = i;

	glProgramUniform1iv(shader_prog, glGetUniformLocation(shader_prog, "textures"), texture_units, texture_indices); GL_DEBUG

	model_shader.prog = shader_prog;
	model_shader.loc_transform = glGetUniformLocation(shader_prog, "model"); GL_DEBUG

	light_shader.prog = shader_prog;
	light_shader_locate(&light_shader);

	return true;
}

void terrain_gfx_deinit()
{
	glDeleteProgram(shader_prog); GL_DEBUG
}

void terrain_gfx_update()
{
	glProgramUniformMatrix4fv(shader_prog, loc_VP, 1, GL_FALSE, frustum[0]); GL_DEBUG
	light_shader_update(&light_shader);
}

void terrain_gfx_make_chunk_model(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;
	pthread_mutex_lock(&chunk->mtx);

	bool animate;
	if (meta->model)
		animate = meta->model->callbacks.step ? true : false;
	else
		animate = !meta->has_model;

	pthread_mutex_unlock(&chunk->mtx);

	bool depends[6] = {false};
	Model *model = create_chunk_model(chunk, animate, depends);

	pthread_mutex_lock(&chunk->mtx);

	if (meta->model) {
		if (model) {
			model->callbacks.step = meta->model->callbacks.step;
			model->root->scale = meta->model->root->scale;
			model_node_transform(model->root);
		}

		meta->model->replace = model;
		meta->model->flags.delete = 1;
	} else if (model) {
		model_scene_add(model);
	}

	meta->model = model;
	meta->has_model = true;

	for (int i = 0; i < 6; i++)
		meta->depends[i] = depends[i];

	pthread_mutex_unlock(&chunk->mtx);
}
