#include <stdlib.h>
#include <perlin/perlin.h>
#include "mapgen.h"

int seed = 0;
static Server *server = NULL;

// mapgen prototype
static void generate_block(MapBlock *block)
{
	for (u8 x = 0; x < 16; x++) {
		u32 ux = x + block->pos.x * 16 + ((u32) 1 << 31);
		for (u8 z = 0; z < 16; z++) {
			u32 uz = z + block->pos.z * 16 + ((u32) 1 << 31);
			s32 height = smooth2d((double) ux / 32.0f, (double) uz / 32.0f, 0, seed) * 16.0f;
			for (u8 y = 0; y < 16; y++) {
				s32 ay = y + block->pos.y * 16;
				Node type;
				if (ay > height)
					type = NODE_AIR;
				else if (ay == height)
					type = NODE_GRASS;
				else if (ay >= height - 4)
					type = NODE_DIRT;
				else
					type = NODE_STONE;
				block->data[x][y][z] = map_node_create(type);
			}
		}
	}
	ITERATE_LIST(&server->clients, pair) {
		Client *client = pair->value;
		if (client->state == CS_ACTIVE) {
			pthread_mutex_lock(&client->mtx);
			(void) (write_u32(client->fd, CC_BLOCK) && map_serialize_block(client->fd, block));
			pthread_mutex_unlock(&client->mtx);
		}
	}
	block->ready = true;
}

void mapgen_init(Server *srv)
{
	server = srv;
	server->map->on_block_create = &generate_block;
}

#define RANGE 3

static void *mapgen_thread(void *cliptr)
{
	Client *client = cliptr;

	while (client->state != CS_DISCONNECTED) {
		v3s32 pos = map_node_to_block_pos((v3s32) {client->pos.x, client->pos.y, client->pos.z}, NULL);
		for (s32 x = pos.x - RANGE; x <= pos.x + RANGE; x++)
			for (s32 y = pos.y - RANGE; y <= pos.y + RANGE; y++)
				for (s32 z = pos.z - RANGE; z <= pos.z + RANGE; z++)
					map_get_block(client->server->map, (v3s32) {x, y, z}, true);
	}

	return NULL;
}

void mapgen_start_thread(Client *client)
{
	(void) client;
	(void) mapgen_thread;
	pthread_t thread;
	pthread_create(&thread, NULL, &mapgen_thread, client);
}
