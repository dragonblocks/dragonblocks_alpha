#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "facecache.h"
#include "map.h"
#include "mapdb.h"
#include "mapgen.h"
#include "servermap.h"

static struct {
	size_t max_blocks;
	pthread_t thread;
	sqlite3 *db;
	bool cancel;
} servermap;

static Server *server = NULL;

static void initialize_block(MapBlock *block)
{
	if (! load_block(servermap.db, block))
		mapgen_generate_block(block);
	block->state = MBS_MODIFIED;
}

static void reset_block(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	if (extra) {
		free(extra->data);
	} else {
		extra = malloc(sizeof(MapBlockExtraData));
		extra->clients = list_create(&list_compare_string);
	}

	map_serialize_block(block, &extra->data, &extra->size);

	block->extra = extra;

	save_block(servermap.db, block);

	ITERATE_LIST(&extra->clients, pair) {
		Client *client = list_get(&server->players, pair->key);
		if (client)
			list_delete(&client->sent_blocks, block);
		free(pair->key);
	}
	list_clear(&extra->clients);

	block->state = MBS_READY;
}

static void send_block(Client *client, MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	list_put(&client->sent_blocks, block, block);
	list_put(&extra->clients, strdup(client->name), NULL);

	pthread_mutex_lock(&client->mtx);
	if (client->state == CS_ACTIVE)
		(void) (write_u32(client->fd, CC_BLOCK) && write_v3s32(client->fd, block->pos) && write(client->fd, extra->data, extra->size) != -1);
	pthread_mutex_unlock(&client->mtx);
}

static void reset_modified_blocks(Client *client)
{
	for (ListPair **pairptr = &client->sent_blocks.first; *pairptr != NULL; pairptr = &(*pairptr)->next) {

		MapBlock *block = (*pairptr)->key;

		pthread_mutex_lock(&block->mtx);
		if (block->state == MBS_MODIFIED) {
			reset_block(block);

			ListPair *next = (*pairptr)->next;
			free(*pairptr);
			*pairptr = next;
		}
		pthread_mutex_unlock(&block->mtx);
	}
}

static void send_blocks(Client *client)
{
	v3s32 pos = map_node_to_block_pos((v3s32) {client->pos.x, client->pos.y, client->pos.z}, NULL);

	for (size_t i = 0; i < servermap.max_blocks; i++) {
		MapBlock *block = map_get_block(client->server->map, get_face(i, &pos), true);

		pthread_mutex_lock(&block->mtx);
		switch (block->state) {
			case MBS_CREATED:

				initialize_block(block);

			__attribute__ ((fallthrough)); case MBS_MODIFIED:

				reset_block(block);

				send:
				send_block(client, block);

				pthread_mutex_unlock(&block->mtx);
				return;

			case MBS_READY:

				if (! list_get(&client->sent_blocks, block))
					goto send;
				else
					break;

			default:

				break;
		}
		pthread_mutex_unlock(&block->mtx);
	}
}

static void map_step()
{
	pthread_rwlock_rdlock(&server->players_rwlck);
	ITERATE_LIST(&server->players, pair) reset_modified_blocks(pair->value);
	ITERATE_LIST(&server->players, pair) send_blocks(pair->value);
	pthread_rwlock_unlock(&server->players_rwlck);
}

static void *map_thread(void *unused)
{
	(void) unused;

	servermap.db = open_mapdb("map.sqlite");

	while (! servermap.cancel)
		map_step();

	return NULL;
}

void servermap_init(Server *srv)
{
	server = srv;
	servermap.max_blocks = get_face_count(3);

	pthread_create(&servermap.thread, NULL, &map_thread, NULL);
}

void servermap_delete_extra_data(void *ext)
{
	MapBlockExtraData *extra = ext;

	if (extra) {
		ITERATE_LIST(&extra->clients, pair) free(pair->key);
		list_clear(&extra->clients);
		free(extra->data);
		free(extra);
	}
}

void servermap_deinit()
{
	servermap.cancel = true;
	pthread_join(servermap.thread, NULL);
	sqlite3_close(servermap.db);
}
