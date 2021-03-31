#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "facecache.h"
#include "mapgen.h"
#include "map.h"
#include "servermap.h"

static size_t max_blocks;

static Server *server = NULL;

typedef struct
{
	char *ptr;
	size_t size;
} MapBlockBuffer;

static void generate_block(MapBlock *block)
{
	pthread_mutex_lock(&block->mtx);
	if (block->state == MBS_CREATED) {
		block->state = MBS_INITIALIZING;
		mapgen_generate_block(block);
		block->state = MBS_UNSENT;
	}
	pthread_mutex_unlock(&block->mtx);
}

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
	if (block->state == MBS_UNSENT) {
		block->state = MBS_SENDING;
		if (block->extra) {
			free(((MapBlockBuffer *) block->extra)->ptr);
			free(block->extra);
			block->extra = NULL;
		}
		// ToDo: only send to near clients
		pthread_rwlock_rdlock(&server->players_rwlck);
		ITERATE_LIST(&server->players, pair) {
			if (block->state != MBS_SENDING)
				break;
			serialize_block(pair->value, block);
		}
		pthread_rwlock_unlock(&server->players_rwlck);
		if (block->state == MBS_SENDING)
			block->state = MBS_READY;
	}
	pthread_mutex_unlock(&block->mtx);
}

static void send_blocks(Client *client)
{
	v3s32 pos = map_node_to_block_pos((v3s32) {client->pos.x, client->pos.y, client->pos.z}, NULL);
	for (size_t i = 0; i < max_blocks; i++) {
		MapBlock *block = map_get_block(client->server->map, get_face(i, &pos), true);
		switch (block->state) {
		case MBS_CREATED:
			generate_block(block);
			__attribute__ ((fallthrough));
		case MBS_UNSENT:
			send_block(block);
			__attribute__ ((fallthrough));
		case MBS_INITIALIZING:
		case MBS_SENDING:
			return;
		case MBS_READY:
			break;
		}
	}
}

static void *map_thread(void *cli)
{
	Client *client = cli;

	while (client->state != CS_DISCONNECTED) {
		send_blocks(client);
		sched_yield();
	}

	return NULL;
}

void servermap_init(Server *srv)
{
	server = srv;
	max_blocks = get_face_count(3);
}

void servermap_add_client(Client *client)
{
	pthread_create(&client->map_thread, NULL, &map_thread, client);
}
