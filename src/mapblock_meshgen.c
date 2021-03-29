#include <stdlib.h>
#include "node.h"
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

static v3f vpos[6][6] = {
	{
		{-0.5f, -0.5f, -0.5f},
		{+0.5f, -0.5f, -0.5f},
		{+0.5f, +0.5f, -0.5f},
		{+0.5f, +0.5f, -0.5f},
		{-0.5f, +0.5f, -0.5f},
		{-0.5f, -0.5f, -0.5f},
	},
	{
		{-0.5f, -0.5f, +0.5f},
		{+0.5f, +0.5f, +0.5f},
		{+0.5f, -0.5f, +0.5f},
		{+0.5f, +0.5f, +0.5f},
		{-0.5f, -0.5f, +0.5f},
		{-0.5f, +0.5f, +0.5f},
	},
	{
		{-0.5f, +0.5f, +0.5f},
		{-0.5f, -0.5f, -0.5f},
		{-0.5f, +0.5f, -0.5f},
		{-0.5f, -0.5f, -0.5f},
		{-0.5f, +0.5f, +0.5f},
		{-0.5f, -0.5f, +0.5f},
	},
	{
		{+0.5f, +0.5f, +0.5f},
		{+0.5f, +0.5f, -0.5f},
		{+0.5f, -0.5f, -0.5f},
		{+0.5f, -0.5f, -0.5f},
		{+0.5f, -0.5f, +0.5f},
		{+0.5f, +0.5f, +0.5f},
	},
	{
		{-0.5f, -0.5f, -0.5f},
		{+0.5f, -0.5f, -0.5f},
		{+0.5f, -0.5f, +0.5f},
		{+0.5f, -0.5f, +0.5f},
		{-0.5f, -0.5f, +0.5f},
		{-0.5f, -0.5f, -0.5f},
	},
	{
		{-0.5f, +0.5f, -0.5f},
		{+0.5f, +0.5f, -0.5f},
		{+0.5f, +0.5f, +0.5f},
		{+0.5f, +0.5f, +0.5f},
		{-0.5f, +0.5f, +0.5f},
		{-0.5f, +0.5f, -0.5f},
	},
};

static v3s8 fdir[6] = {
	{+0, +0, -1},
	{+0, +0, +1},
	{-1, +0, +0},
	{+1, +0, +0},
	{+0, -1, +0},
	{+0, +1, +0},
};

#define GNODDEF(block, x, y, z) node_definitions[block->data[x][y][z].type]
#define VALIDPOS(pos) (pos.x >= 0 && pos.x < 16 && pos.y >= 0 && pos.y < 16 && pos.z >= 0 && pos.z < 16)

static Array make_vertices(MapBlock *block)
{
	Array vertices = array_create(sizeof(Vertex));

	ITERATE_MAPBLOCK {
		NodeDefintion *def = &GNODDEF(block, x, y, z);
		if (def->visible) {
			v3u8 pos = {x, y, z};
			v3f offset = {x + 8.5f, y + 8.5f, z + 8.5f};
			v3f color = get_node_color(def);
			for (int f = 0; f < 6; f++) {
				v3s8 *noff = &fdir[f];
				v3s8 npos = {
					pos.x + noff->x,
					pos.y + noff->y,
					pos.z + noff->z,
				};
				if (! VALIDPOS(npos) || ! GNODDEF(block, npos.x, npos.y, npos.z).visible) {
					for (int v = 0; v < 6; v++) {
						Vertex vertex = {
							vpos[f][v].x + offset.x,
							vpos[f][v].y + offset.y,
							vpos[f][v].z + offset.z,
							color.x,
							color.y,
							color.z,
						};
						array_append(&vertices, &vertex);
					}
				}
			}
		}
	}

	return vertices;
}

#undef GNODDEF
#undef VALIDPOS

static void *meshgen_thread(void *unused)
{
	(void) unused;

	while (! meshgen.cancel) {
		ListPair **lptr = &meshgen.queue.first;
		if (*lptr) {
			MapBlock *block = (*lptr)->key;

			pthread_mutex_lock(&meshgen.mtx);
			ListPair *next = (*lptr)->next;
			free(*lptr);
			*lptr = next;
			pthread_mutex_unlock(&meshgen.mtx);

			Array vertices = make_vertices(block);
			Mesh *mesh = NULL;

			if (vertices.siz > 0) {
				mesh = mesh_create(vertices.ptr, vertices.siz);
				mesh->pos = (v3f) {block->pos.x * 16.0f - 8.0f, block->pos.y * 16.0f - 8.0f, block->pos.z * 16.0f - 8.0f};
				mesh_transform(mesh);
				scene_add_mesh(meshgen.scene, mesh);
			}

			if (block->extra)
				((Mesh *) block->extra)->remove = true;

			block->extra = mesh;
		} else {
			sched_yield();
		}
	}

	return NULL;
}

static void enqueue_block(MapBlock *block)
{
	pthread_mutex_lock(&meshgen.mtx);
	list_put(&meshgen.queue, block, NULL);
	pthread_mutex_unlock(&meshgen.mtx);
}

void mapblock_meshgen_init(Map *map, Scene *scene)
{
	meshgen.map = map;
	meshgen.scene = scene;
	meshgen.queue = list_create(NULL);
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
