#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <perlin/perlin.h>
#include "servermap.h"

int seed = 0;
static Server *server = NULL;

static void generate_block(MapBlock *block)
{
	pthread_mutex_lock(&block->mtx);
	if (block->state != MBS_CREATED)
		return;
	block->state = MBS_PROCESSING;
	pthread_mutex_unlock(&block->mtx);
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
				block->metadata[x][y][z] = list_create(&list_compare_string);
			}
		}
	}
	block->state = MBS_READY;
}

typedef struct
{
	char *ptr;
	size_t size;
} MapBlockBuffer;

static void serialize_block(Client *client, MapBlock *block)
{
	MapBlockBuffer *buffer = block->extra;

	if (! buffer) {
		buffer = malloc(sizeof(MapBlockBuffer));

		FILE *buffile = open_memstream(&buffer->ptr, &buffer->size);
		map_serialize_block(buffile, block);
		fflush(buffile);
		fclose(buffile);

		block->extra = buffer;
	}

	pthread_mutex_lock(&client->mtx);
	(void) (write_u32(client->fd, CC_BLOCK) && write(client->fd, buffer->ptr, buffer->size) != -1);
	pthread_mutex_unlock(&client->mtx);
}

static void send_block(MapBlock *block)
{
	pthread_mutex_lock(&block->mtx);
	block->state = MBS_READY;
	// ToDo: only send to near clients
	ITERATE_LIST(&server->clients, pair) {
		if (block->state != MBS_READY)
			break;
		Client *client = pair->value;
		if (client->state == CS_ACTIVE)
			serialize_block(client, block);
	}
	pthread_mutex_unlock(&block->mtx);
}

#define RANGE 3
#define POS_CACHE_COUNT ((1 + RANGE * 2) * (1 + RANGE * 2) * (1 + RANGE * 2))
static size_t pos_chache_count = POS_CACHE_COUNT;
static v3s32 pos_cache[POS_CACHE_COUNT];

static void init_pos_cache()
{
	size_t i = -1;
#define ADDPOS(a, b, c, va, vb, vc) \
	*(s32 *) ((char *) &pos_cache[++i] + offsetof(v3s32, a)) = va; \
	*(s32 *) ((char *) &pos_cache[i] + offsetof(v3s32, b)) = vb; \
	*(s32 *) ((char *) &pos_cache[i] + offsetof(v3s32, c)) = vc;
	ADDPOS(x, y, z, 0, 0, 0)
	for (s32 l = 1; l <= RANGE ; l++) {
#define SQUARES(a, b, c) \
		for (s32 va = -l + 1; va < l; va++) { \
			for (s32 vb = -l + 1; vb < l; vb++) { \
				ADDPOS(a, b, c, va, vb,  l) \
				ADDPOS(a, b, c, va, vb, -l) \
			} \
		}
		SQUARES(x, z, y)
		SQUARES(x, y, z)
		SQUARES(z, y, x)
#undef SQUARES
#define EDGES(a, b, c) \
		for (s32 va = -l + 1; va < l; va++) { \
			ADDPOS(a, b, c, va,  l,  l) \
			ADDPOS(a, b, c, va,  l, -l) \
			ADDPOS(a, b, c, va, -l,  l) \
			ADDPOS(a, b, c, va, -l, -l) \
		}
		EDGES(x, y, z)
		EDGES(z, x, y)
		EDGES(y, x, z)
#undef EDGES
		ADDPOS(x, y, z,  l,  l,  l)
		ADDPOS(x, y, z,  l,  l, -l)
		ADDPOS(x, y, z,  l, -l,  l)
		ADDPOS(x, y, z,  l, -l, -l)
		ADDPOS(x, y, z, -l,  l,  l)
		ADDPOS(x, y, z, -l,  l, -l)
		ADDPOS(x, y, z, -l, -l,  l)
		ADDPOS(x, y, z, -l, -l, -l)
#undef ADDPOS
	}
}

static void send_blocks(Client *client, bool init)
{
	v3s32 pos = map_node_to_block_pos((v3s32) {client->pos.x, client->pos.y, client->pos.z}, NULL);
	for (size_t i = 0; i < pos_chache_count; i++) {
		MapBlock *block = map_get_block(client->server->map, (v3s32) {pos.x + pos_cache[i].x, pos.y + pos_cache[i].y, pos.z + pos_cache[i].z}, ! init);
		if (init) {
			if (block)
				serialize_block(client, block);
		} else switch (block->state) {
		case MBS_CREATED:
			generate_block(block);
			__attribute__ ((fallthrough));
		case MBS_MODIFIED:
			if (block->extra) {
				free(((MapBlockBuffer *) block->extra)->ptr);
				free(block->extra);
				block->extra = NULL;
			}
			send_block(block);
			__attribute__ ((fallthrough));
		case MBS_PROCESSING:
			i = -1;
			sched_yield();
			__attribute__ ((fallthrough));
		case MBS_READY:
			break;
		}
	}
}

static void *block_send_thread(void *cli)
{
	Client *client = cli;

	send_blocks(client, true);

	while (client->state != CS_DISCONNECTED)
		send_blocks(client, false);

	return NULL;
}

void servermap_init(Server *srv)
{
	server = srv;
	init_pos_cache();
}

void servermap_add_client(Client *client)
{
	pthread_t thread;
	pthread_create(&thread, NULL, &block_send_thread, client);
}

#include <perlin/perlin.c>
