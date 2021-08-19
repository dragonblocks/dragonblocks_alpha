#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "server/mapdb.h"
#include "server/mapgen.h"
#include "server/server_map.h"
#include "map.h"
#include "util.h"

struct ServerMap server_map;
static Server *server;

// utility functions

// send a block to a client and reset block request
static void send_block(Client *client, MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	pthread_mutex_lock(&client->mtx);
	if (client->state == CS_ACTIVE)
		(void) (write_u32(client->fd, CC_BLOCK) && write_v3s32(client->fd, block->pos) && write_u64(client->fd, extra->size) && write(client->fd, extra->data, extra->size) != -1);
	pthread_mutex_unlock(&client->mtx);
}

// send block to near clients
// block mutex has to be locked
static void send_block_to_near(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	if (extra->state == MBS_GENERATING)
		return;

	map_serialize_block(block, &extra->data, &extra->size);
	mapdb_save_block(server_map.db, block);

	if (extra->state == MBS_CREATED)
		return;

	pthread_rwlock_rdlock(&server->players_rwlck);
	ITERATE_LIST(&server->players, pair) {
		Client *client = pair->value;

		if (within_simulation_distance(client->pos, block->pos, server->config.simulation_distance))
			send_block(client, block);
	}
	pthread_rwlock_unlock(&server->players_rwlck);
}

// list_clear_func callback for sending changed blocks to near clients
static void list_send_block(void *key, __attribute__((unused)) void *value, __attribute__((unused)) void *arg)
{
	MapBlock *block = key;

	pthread_mutex_lock(&block->mtx);
	send_block_to_near(block);
	pthread_mutex_unlock(&block->mtx);
}

// pthread start routine for mapgen thread
static void *mapgen_thread(void *arg)
{
	MapBlock *block = arg;
	MapBlockExtraData *extra = block->extra;

	pthread_mutex_lock(&block->mtx);
	extra->state = MBS_GENERATING;
	pthread_mutex_unlock(&block->mtx);

	List changed_blocks = list_create(NULL);
	list_put(&changed_blocks, block, NULL);

	mapgen_generate_block(block, &changed_blocks);

	pthread_mutex_lock(&block->mtx);
	extra->state = MBS_READY;
	pthread_mutex_unlock(&block->mtx);

	list_clear_func(&changed_blocks, &list_send_block, NULL);

	if (! server_map.shutting_down) {
		pthread_mutex_lock(&server_map.mapgen_threads_mtx);
		list_delete(&server_map.mapgen_threads, &extra->mapgen_thread);
		pthread_mutex_unlock(&server_map.mapgen_threads_mtx);
	}

	return NULL;
}

// launch mapgen thread for block
// block mutex has to be locked
static void launch_mapgen_thread(MapBlock *block)
{
	pthread_mutex_lock(&server_map.mapgen_threads_mtx);
	pthread_t *thread_ptr = &((MapBlockExtraData *) block->extra)->mapgen_thread;
	pthread_create(thread_ptr, NULL, mapgen_thread, block);
	list_put(&server_map.mapgen_threads, thread_ptr, NULL);
	pthread_mutex_unlock(&server_map.mapgen_threads_mtx);
}

// list_clear_func callback used to join running generator threads on shutdown
static void list_join_thread(void *key, __attribute__((unused)) void *value, __attribute__((unused)) void *arg)
{
	pthread_join(*(pthread_t *) key, NULL);
}

// map callbacks
// note: all these functions require the block mutex to be locked, which is always the case when a map callback is invoked

// callback for initializing a newly created block
// load block from database or initialize state, mgstage buffer and data
static void on_create_block(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra = malloc(sizeof(MapBlockExtraData));

	if (! mapdb_load_block(server_map.db, block)) {
		extra->state = MBS_CREATED;
		extra->data = NULL;

		ITERATE_MAPBLOCK {
			block->data[x][y][z] = map_node_create(NODE_AIR);
			block->metadata[x][y][z] = list_create(&list_compare_string);
			extra->mgs_buffer[x][y][z] = MGS_VOID;
		}
	}
}

// callback for deleting a block
// free extra data
static void on_delete_block(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	if (extra->data)
		free(extra->data);

	free(extra);
}

// callback for determining whether a block should be returned by map_get_block
// hold back blocks that are not fully generated except when the create flag is set to true
static bool on_get_block(MapBlock *block, bool create)
{
	MapBlockExtraData *extra = block->extra;

	if (extra->state < MBS_READY && ! create)
		return false;

	return true;
}

// callback for deciding whether a set_node call succeeds or not
// reject set_node calls that try to override nodes placed by later mapgen stages, else update mgs buffer - also make sure block is inserted into changed blocks list
static bool on_set_node(MapBlock *block, v3u8 offset, __attribute__((unused)) MapNode *node, void *arg)
{
	MapgenSetNodeArg *msn_arg = arg;

	MapgenStage mgs;

	if (msn_arg)
		mgs = msn_arg->mgs;
	else
		mgs = MGS_PLAYER;

	MapgenStage *old_mgs = &((MapBlockExtraData *) block->extra)->mgs_buffer[offset.x][offset.y][offset.z];

	if (mgs >= *old_mgs) {
		*old_mgs = mgs;

		if (msn_arg)
			list_put(msn_arg->changed_blocks, block, NULL);

		return true;
	}

	return false;
}

// callback for when a block changes
// send block to near clients if not part of map generation
static void on_after_set_node(MapBlock *block, __attribute__((unused)) v3u8 offset, void *arg)
{
	if (! arg)
		send_block_to_near(block);
}

// public functions

// ServerMap singleton constructor
void server_map_init(Server *srv)
{
	server = srv;

	server_map.map = map_create((MapCallbacks) {
		.create_block = &on_create_block,
		.delete_block = &on_delete_block,
		.get_block = &on_get_block,
		.set_node = &on_set_node,
		.after_set_node = &on_after_set_node,
	});
	server_map.shutting_down = false;
	server_map.db = mapdb_open("map.sqlite");
	server_map.mapgen_threads = list_create(NULL);
	pthread_mutex_init(&server_map.mapgen_threads_mtx, NULL);
}

// ServerMap singleton destructor
void server_map_deinit()
{
	server_map.shutting_down = true;

	pthread_mutex_lock(&server_map.mapgen_threads_mtx);
	list_clear_func(&server_map.mapgen_threads, &list_join_thread, NULL);
	// pthread_mutex_unlock(&server_map.mapgen_threads_mtx);
	pthread_mutex_destroy(&server_map.mapgen_threads_mtx);

	sqlite3_close(server_map.db);
	map_delete(server_map.map);
}

// handle block request from client (thread safe)
void server_map_requested_block(Client *client, v3s32 pos)
{
	if (within_simulation_distance(client->pos, pos, server->config.simulation_distance)) {
		MapBlock *block = map_get_block(server_map.map, pos, true);

		pthread_mutex_lock(&block->mtx);
		MapBlockExtraData *extra = block->extra;

		switch (extra->state) {
			case MBS_CREATED:
				launch_mapgen_thread(block);
				break;

			case MBS_GENERATING:
				break;

			case MBS_READY:
				send_block(client, block);
		};
		pthread_mutex_unlock(&block->mtx);
	}
}
