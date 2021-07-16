#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "server/facecache.h"
#include "server/mapdb.h"
#include "server/mapgen.h"
#include "server/server_map.h"
#include "map.h"

static struct {
	Server *server;
	size_t max_blocks;
	pthread_t thread;
	sqlite3 *db;
	bool cancel;
} server_map;

static void initialize_block(MapBlock *block)
{
	if (! mapdb_load_block(server_map.db, block))
		mapgen_generate_block(block);
	block->state = MBS_MODIFIED;
}

static void reset_client_block(void *key, __attribute__((unused)) void *value, void *block)
{
	Client *client = list_get(&server_map.server->players, key);
	if (client)
		list_delete(&client->sent_blocks, block);
	free(key);
}

static void list_delete_extra_data(void *key, __attribute__((unused)) void *value, __attribute__((unused)) void *unused)
{
	free(key);
}

static void free_extra_data(void *ext)
{
	MapBlockExtraData *extra = ext;

	if (extra) {
		list_clear_func(&extra->clients, &list_delete_extra_data, NULL);
		free(extra->data);
		free(extra);
	}
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
	block->free_extra = &free_extra_data;

	mapdb_save_block(server_map.db, block);

	list_clear_func(&extra->clients, &reset_client_block, block);
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

	for (size_t i = 0; i < server_map.max_blocks; i++) {
		MapBlock *block = map_get_block(client->server->map, facecache_face(i, &pos), true);

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
	pthread_rwlock_rdlock(&server_map.server->players_rwlck);
	ITERATE_LIST(&server_map.server->players, pair) reset_modified_blocks(pair->value);
	ITERATE_LIST(&server_map.server->players, pair) send_blocks(pair->value);
	pthread_rwlock_unlock(&server_map.server->players_rwlck);
}

static void *map_thread(__attribute__((unused)) void *unused)
{
	server_map.db = mapdb_open("map.sqlite");

	while (! server_map.cancel)
		map_step();

	return NULL;
}

void server_map_init(Server *server)
{
	server_map.server = server;
	server_map.max_blocks = facecache_count(16);

	pthread_create(&server_map.thread, NULL, &map_thread, NULL);
}

void server_map_deinit()
{
	server_map.cancel = true;
	pthread_join(server_map.thread, NULL);
	sqlite3_close(server_map.db);
}
