#include <assert.h>
#include <linmath.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/client_config.h"
#include "client/client_node.h"
#include "client/client_terrain.h"
#include "client/cube.h"
#include "client/frustum.h"
#include "client/opengl.h"
#include "client/light.h"
#include "client/shader.h"
#include "client/terrain_gfx.h"
#include "common/facedir.h"

typedef struct {
	TerrainChunk *chunk;     // input: chunk pointer
	TerrainChunkMeta *meta;  // input: coersed chunk metadata pointer
	v3s32 chunkp;            // input: position of chunk
	bool animate;            // input: disable edge culling
	Array vertices[2];       // main output: vertex data, 0=regular, 1=transparent textures
	bool abort;              // output: state changes have occured that invalidate generated output
	bool grabbed[6];         // output: neighbors that have been grabbed
	bool visible;            // output: edge culled model would be visible
	bool remake_needed;      // output: edge culled model would be different from non-culled
} ChunkRenderData;

static VertexLayout terrain_vertex_layout = {
	.attributes = (VertexAttribute []) {
		{GL_FLOAT, 3, sizeof(v3f32)}, // position
		{GL_FLOAT, 3, sizeof(v3f32)}, // normal
		{GL_FLOAT, 2, sizeof(v2f32)}, // textureCoordinates
		{GL_FLOAT, 3, sizeof(v3f32)}, // color
	},
	.count = 4,
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

static void grab_neighbor(ChunkRenderData *data, int i)
{
	// return if we've already subscribed/grabbed the lock
	if (data->meta->depends[i])
		return;

	// we are now subscribed to state changes from the neighbor
	data->meta->depends[i] = true;

	TerrainChunk *neighbor = data->meta->neighbors[i];
	TerrainChunkMeta *neighbor_meta = neighbor->extra;

	pthread_rwlock_rdlock(&neighbor_meta->lock_state);
	// check neighbor in case it was already in a bad state before we subscribed
	if ((data->grabbed[i] = neighbor_meta->state > CHUNK_STATE_RECV))
		// if state is good, actually grab the data lock in read mode
		assert(pthread_rwlock_rdlock(&neighbor->lock) == 0);
	else
		// if state is bad, set flag to abort
		data->abort = true;
	pthread_rwlock_unlock(&neighbor_meta->lock_state);
}

static inline bool show_face(ChunkRenderData *data, NodeArgsRender *args, v3s32 offset)
{
	NodeVisibility visibility = client_node_def[args->node->type].visibility;

	// always render clip nodes
	if (visibility == VISIBILITY_CLIP)
		return data->visible = true;

	// calculate offset of neighbor node from current chunk
	v3s32 nbr_offset = v3s32_add(offset, facedir[args->f]);

	// if offset is outside bounds, we'll have to select a neighbor chunk
	bool nbr_chunk =
		nbr_offset.x < 0 || nbr_offset.x >= CHUNK_SIZE ||
		nbr_offset.y < 0 || nbr_offset.y >= CHUNK_SIZE ||
		nbr_offset.z < 0 || nbr_offset.z >= CHUNK_SIZE;

	NodeType nbr_node = NODE_UNKNOWN;

	if (nbr_chunk) {
		// grab neighbor chunk data lock
		grab_neighbor(data, args->f);

		// if grabbing failed, return true so caller immediately takes notice of abort
		if (data->abort)
			return true;

		nbr_offset = terrain_offset(nbr_offset);

		// select node from neighbor chunk
		nbr_node = data->meta->neighbors[args->f]->data
			[nbr_offset.x][nbr_offset.y][nbr_offset.z].type;
	} else {
		// select node from current chunk
		nbr_node = data->chunk->data[nbr_offset.x][nbr_offset.y][nbr_offset.z].type;
	}

	if (visibility == VISIBILITY_BLEND) {
		// liquid nodes only render faces towards 'outsiders'
		if (nbr_node != args->node->type)
			return data->visible = true;
	} else { // visibility == VISIBILITY_SOLID
		// faces between two solid nodes are culled
		if (client_node_def[nbr_node].visibility != VISIBILITY_SOLID)
			return data->visible = true;

		// if the chunk needs to be animated, dont cull faces to nodes in other chunks
		// but remember to rebuild the chunk model after the animation has finished
		if (nbr_chunk && data->animate)
			return data->remake_needed = true;
	}

	return false;
}

static inline void render_node(ChunkRenderData *data, v3s32 offset)
{
	NodeArgsRender args;

	args.node = &data->chunk->data[offset.x][offset.y][offset.z];

	ClientNodeDef *def = &client_node_def[args.node->type];

	if (def->visibility == VISIBILITY_NONE)
		return;

	v3f32 vertex_offset = v3f32_sub(v3s32_to_f32(offset), center_offset);
	if (def->render)
		args.pos = v3s32_add(offset, data->chunkp);

	for (args.f = 0; args.f < 6; args.f++) {
		if (!show_face(data, &args, offset))
			continue;

		if (data->abort)
			return;

		for (args.v = 0; args.v < 6; args.v++) {
			args.vertex.cube = cube_vertices[args.f][args.v];
			args.vertex.cube.position = v3f32_add(args.vertex.cube.position, vertex_offset);
			args.vertex.color = (v3f32) {1.0f, 1.0f, 1.0f};

			if (def->render)
				def->render(&args);

			TextureSlice *texture = &def->tiles.textures[args.f];
			v2f32 *tcoord = &args.vertex.cube.textureCoordinates;
			tcoord->x = texture->tex_coord_x + tcoord->x * texture->tex_coord_w;
			tcoord->y = texture->tex_coord_y + tcoord->y * texture->tex_coord_h;

			array_apd(&data->vertices[def->visibility == VISIBILITY_BLEND], &args.vertex);
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

		if (model->extra)
			client_terrain_meshgen_task(model->extra, false);
	}
}

static Model *create_chunk_model(ChunkRenderData *data)
{
	if (!data->visible || (!data->vertices[0].siz && !data->vertices[1].siz))
		return NULL;

	Model *model = model_create();

	if (data->remake_needed)
		model->extra = data->chunk;

	model->box = (aabb3f32) {
		v3f32_sub((v3f32) {-1.0f, -1.0f, -1.0f}, center_offset),
		v3f32_add((v3f32) {+1.0f, +1.0f, +1.0f}, center_offset)};

	model->callbacks.step = data->animate ? &animate_chunk_model : NULL;
	model->callbacks.delete = &model_free_meshes;
	model->flags.frustum_culling = 1;
	model->flags.transparent = data->vertices[1].siz > 0;

	model->root->pos = v3f32_add(v3s32_to_f32(data->chunkp), center_offset);
	model->root->scale = data->animate ? (v3f32) {0.0f, 0.0f, 0.0f} : (v3f32) {1.0f, 1.0f, 1.0f};

	for (size_t i = 0; i < 2; i++) {
		if (!data->vertices[i].siz) continue;

		Mesh *mesh = calloc(1, sizeof *mesh);
		mesh->layout = &terrain_vertex_layout;
		mesh->data = data->vertices[i].ptr;
		mesh->count = data->vertices[i].siz;
		mesh->free_data = true;

		model_node_add_mesh(model->root, &(ModelMesh) {
			.mesh = mesh,
			.textures = &client_node_atlas.txo,
			.num_textures = 1,
			.shader = &model_shader,
		});
	}

	return model;
}

void terrain_gfx_init()
{
	char *shader_def;
	asprintf(&shader_def,
		"#define VIEW_DISTANCE %lf\n",
		client_config.view_distance
	);

	shader_prog = shader_program_create(ASSET_PATH "shaders/3d/terrain", shader_def);
	free(shader_def);

	loc_VP = glGetUniformLocation(shader_prog, "VP"); GL_DEBUG

	model_shader.prog = shader_prog;
	model_shader.loc_transform = glGetUniformLocation(shader_prog, "model"); GL_DEBUG

	light_shader.prog = shader_prog;
	light_shader_locate(&light_shader);
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
	// type coersion
	TerrainChunkMeta *meta = chunk->extra;

	// lock model mutex
	pthread_mutex_lock(&meta->mtx_model);

	// giving 10 arguments to a function is slow and unmaintainable, use pointer to struct instead
	ChunkRenderData data = {
		.chunk = chunk,
		.meta = meta,
		.chunkp = v3s32_scale(chunk->pos, CHUNK_SIZE),
		.animate = false,
		.vertices = {},
		.abort = false,
		.grabbed = {false},
		.visible = false,
		.remake_needed = false,
	};

	array_ini(&data.vertices[0], sizeof(TerrainVertex), 10000);
	array_ini(&data.vertices[1], sizeof(TerrainVertex), 10000);

	//  animate if old animation hasn't finished (or this is the first model)
	if (meta->model)
		data.animate = meta->model->callbacks.step ? true : false;
	else
		data.animate = !meta->has_model;

	// obtain own data lock
	assert(pthread_rwlock_rdlock(&chunk->lock) == 0);

	// clear dependencies, they are repopulated by calls to grab_neighbor
	for (int i = 0; i < 6; i++)
		meta->depends[i] = false;

	// render nodes
	CHUNK_ITERATE {
		// obtain changed state
		pthread_rwlock_rdlock(&meta->lock_state);
		data.abort = meta->state < CHUNK_STATE_CLEAN;
		pthread_rwlock_unlock(&meta->lock_state);

		// abort if chunk has been changed
		// just "break" won't work, the CHUNK_ITERATE macro is a nested loop
		if (data.abort)
			goto abort;

		// make vertex data
		render_node(&data, (v3s32) {x, y, z});

		// abort if failed to grab a neighbor
		if (data.abort)
			goto abort;
	}
	abort:

	// release grabbed data locks
	for (int i = 0; i < 6; i++)
		if (data.grabbed[i])
			pthread_rwlock_unlock(&meta->neighbors[i]->lock);

	// release own data lock
	pthread_rwlock_unlock(&chunk->lock);

	// only create model if we didn't abort
	Model *model = data.abort ? NULL : create_chunk_model(&data);

	// make sure to free vertex mem if it wasn't fed into model
	if (!model) {
		array_clr(&data.vertices[0]);
		array_clr(&data.vertices[1]);
	}

	// abort if chunk changed
	if (data.abort) {
		pthread_mutex_unlock(&meta->mtx_model);
		return;
	}

	// replace old model
	if (meta->model) {
		if (model) {
			// copy animation callback
			model->callbacks.step = meta->model->callbacks.step;
			// copy scale
			model->root->scale = meta->model->root->scale;
			model_node_transform(model->root);
		}

		// if old model wasn't drawn in this frame yet, new model will be drawn instead
		meta->model->replace = model;
		meta->model->flags.delete = 1;
	} else if (model) {
		// model will be drawn in next frame
		model_scene_add(model);
		model_node_transform(model->root);
	}

	// update pointers
	meta->model = model;
	meta->has_model = true;

	// bye bye
	pthread_mutex_unlock(&meta->mtx_model);
}
