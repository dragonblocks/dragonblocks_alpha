#include <stdlib.h>
#include "mapblock_meshgen.h"

static struct
{
	Map *map;
	Scene *scene;
	List queue;
	pthread_mutex_t mtx;
	pthread_t thread;
	bool cancel;
} meshgen;

static Array make_vertices(MapBlock *block)
{
	Array vertices = array_create(sizeof(GLfloat));

	(void) block;

	/*
	ITERATE_MAPBLOCK {
		MapNode node = block->data[x][y][z];
		BlockDef *def = block->getDef();
		if (! def->drawable)
			continue;
		ivec3 bpos(x, y, z);
		vec3 pos_from_mesh_origin = vec3(bpos) - vec3(SIZE / 2 + 0.5);
		for (int facenr = 0; facenr < 6; facenr++) {
			ivec3 npos = bpos + face_dir[facenr];
			const Block *neighbor_own, *neighbor;
			neighbor_own = neighbor = getBlockNoEx(npos);
			if (! neighbor)
				neighbor = map->getBlock(pos * SIZE + npos);
			if (neighbor && ! neighbor->getDef()->drawable)
				any_drawable_block = true;
			if (! mesh_created_before)
				neighbor = neighbor_own;
			if (! mesh_created_before && ! neighbor || neighbor && ! neighbor->getDef()->drawable) {
				textures.push_back(def->tile_def.get(facenr));
				for (int vertex_index = 0; vertex_index < 6; vertex_index++) {
					for (int attribute_index = 0; attribute_index < 5; attribute_index++) {
						GLdouble value = box_vertices[facenr][vertex_index][attribute_index];
						switch (attribute_index) {
							case 0:
							value += pos_from_mesh_origin.x;
							break;
							case 1:
							value += pos_from_mesh_origin.y;
							break;
							case 2:
							value += pos_from_mesh_origin.z;
							break;
						}
						vertices.push_back(value);
					}
				}
			}
		}
	}
	*/

	return vertices;
}

static void *meshgen_thread(void *unused)
{
	(void) unused;

	while (! meshgen.cancel) {
		ListPair **lptr = &meshgen.queue.first;
		if (*lptr) {
			MapBlock *block = map_get_block(meshgen.map, *(v3s32 *)(*lptr)->key, false);

			pthread_mutex_lock(&meshgen.mtx);
			free((*lptr)->key);
			*lptr = (*lptr)->next;
			pthread_mutex_unlock(&meshgen.mtx);

			Array vertices = make_vertices(block);
			Mesh *mesh = NULL;

			if (vertices.siz > 0) {
				mesh = mesh_create(vertices.ptr, vertices.siz);
				scene_add_mesh(meshgen.scene, mesh);
			}

			if (block->extra)
				mesh->remove = true;

			block->extra = mesh;
		} else {
			sched_yield();
		}
	}

	return NULL;
}

static void enqueue_block(MapBlock *block)
{
	v3s32 *posptr = malloc(sizeof(v3s32));
	*posptr = block->pos;
	pthread_mutex_lock(&meshgen.mtx);
	if (! list_put(&meshgen.queue, posptr, NULL))
		free(posptr);
	pthread_mutex_unlock(&meshgen.mtx);
}

static bool compare_positions(void *p1, void *p2)
{
	v3s32 *pos1 = p1;
	v3s32 *pos2 = p2;
	return pos1->x == pos2->x && pos1->y == pos2->y && pos1->z == pos2->z;
}

void mapblock_meshgen_init(Map *map, Scene *scene)
{
	meshgen.map = map;
	meshgen.scene = scene;
	meshgen.queue = list_create(&compare_positions);
	pthread_mutex_init(&meshgen.mtx, NULL);
	map->on_block_add = &enqueue_block;
	map->on_block_change = &enqueue_block;
	pthread_create(&meshgen.thread, NULL, &meshgen_thread, NULL);
}

void mapblock_meshgen_stop()
{
	meshgen.cancel = true;
	pthread_join(meshgen.thread, NULL);
	pthread_mutex_destroy(&meshgen.mtx);
	ITERATE_LIST(&meshgen.queue, pair) free(pair->key);
	list_clear(&meshgen.queue);
}
