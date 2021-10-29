#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "map.h"
#include "server/database.h"
#include "server/mapgen.h"
#include "server/server_map.h"
#include "signal_handlers.h"
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

	if (extra->data)
		free(extra->data);

	map_serialize_block(block, &extra->data, &extra->size);
	database_save_block(block);

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
static void list_send_block(void *key, unused void *value, unused void *arg)
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

	pthread_mutex_lock(&server_map.joining_threads_mtx);
	if (! server_map.joining_threads) {
		pthread_mutex_lock(&server_map.mapgen_threads_mtx);
		list_delete(&server_map.mapgen_threads, &extra->mapgen_thread);
		pthread_mutex_unlock(&server_map.mapgen_threads_mtx);
	}
	pthread_mutex_unlock(&server_map.joining_threads_mtx);

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
static void list_join_thread(void *key, unused void *value, unused void *arg)
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

	if (! database_load_block(block)) {
		extra->state = MBS_CREATED;
		extra->data = NULL;

		ITERATE_MAPBLOCK {
			block->data[x][y][z] = map_node_create(NODE_AIR);
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
static bool on_set_node(MapBlock *block, v3u8 offset, unused MapNode *node, void *arg)
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
static void on_after_set_node(MapBlock *block, unused v3u8 offset, void *arg)
{
	if (! arg)
		send_block_to_near(block);
}

// join all map generation threads
static void join_mapgen_threads()
{
	pthread_mutex_lock(&server_map.joining_threads_mtx);
	server_map.joining_threads = true;
	pthread_mutex_unlock(&server_map.joining_threads_mtx);

	pthread_mutex_lock(&server_map.mapgen_threads_mtx);
	list_clear_func(&server_map.mapgen_threads, &list_join_thread, NULL);
	pthread_mutex_unlock(&server_map.mapgen_threads_mtx);

	pthread_mutex_lock(&server_map.joining_threads_mtx);
	server_map.joining_threads = false;
	pthread_mutex_unlock(&server_map.joining_threads_mtx);
}

// generate a hut for new players to spawn in
static void generate_spawn_hut()
{
	List changed_blocks = list_create(NULL);

	for (s32 x = -4; x <= +4; x++) {
		for (s32 y = 0; y <= 3; y++) {
			for (s32 z = -3; z <= +2; z++) {
				mapgen_set_node((v3s32) {x, server_map.spawn_height + y, z}, (MapNode) {NODE_AIR}, MGS_PLAYER, &changed_blocks);
			}
		}
	}

	for (s32 x = -5; x <= +5; x++) {
		for (s32 z = -4; z <= +3; z++) {
			mapgen_set_node((v3s32) {x, server_map.spawn_height - 1, z}, (MapNode) {NODE_WOOD}, MGS_PLAYER, &changed_blocks);
			mapgen_set_node((v3s32) {x, server_map.spawn_height + 4, z}, (MapNode) {NODE_WOOD}, MGS_PLAYER, &changed_blocks);
		}
	}

	for (s32 y = 0; y <= 3; y++) {
		for (s32 x = -5; x <= +5; x++) {
			mapgen_set_node((v3s32) {x, server_map.spawn_height + y, -4}, (MapNode) {((y == 1 || y == 2) && ((x >= -3 && x <= -1) || (x >= +1 && x <= +2))) ? NODE_AIR : NODE_WOOD}, MGS_PLAYER, &changed_blocks);
			mapgen_set_node((v3s32) {x, server_map.spawn_height + y, +3}, (MapNode) {((y == 1 || y == 2) && ((x >= -3 && x <= -2) || (x >= +1 && x <= +3))) ? NODE_AIR : NODE_WOOD}, MGS_PLAYER, &changed_blocks);
		}
	}

	for (s32 y = 0; y <= 3; y++) {
		for (s32 z = -3; z <= +2; z++) {
			mapgen_set_node((v3s32) {-5, server_map.spawn_height + y, z}, (MapNode) {NODE_WOOD}, MGS_PLAYER, &changed_blocks);
			mapgen_set_node((v3s32) {+5, server_map.spawn_height + y, z}, (MapNode) {((y != 3) && (z == -1 || z == +0)) ? NODE_AIR : NODE_WOOD}, MGS_PLAYER, &changed_blocks);
		}
	}

	v2s32 posts[6] = {
		{-4, -3},
		{-4, +2},
		{+4, -3},
		{+4, +2},
		{+5, -1},
		{+5, +0},
	};

	for (int i = 0; i < 6; i++) {
		for (s32 y = server_map.spawn_height - 2;; y--) {
			v3s32 pos = {posts[i].x, y, posts[i].y};
			Node node = map_get_node(server_map.map, pos).type;

			if (i >= 4) {
				if (node != NODE_AIR)
					break;

				pos.y++;
			}

			if (node_definitions[node].solid)
				break;

			mapgen_set_node(pos, (MapNode) {node == NODE_LAVA ? NODE_VULCANO_STONE : NODE_WOOD}, MGS_PLAYER, &changed_blocks);
		}
	}

	list_clear_func(&changed_blocks, &list_send_block, NULL);
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
	server_map.joining_threads = false;
	server_map.mapgen_threads = list_create(NULL);
	pthread_mutex_init(&server_map.joining_threads_mtx, NULL);
	pthread_mutex_init(&server_map.mapgen_threads_mtx, NULL);
}

// ServerMap singleton destructor
void server_map_deinit()
{
	join_mapgen_threads();
	pthread_mutex_destroy(&server_map.joining_threads_mtx);
	pthread_mutex_destroy(&server_map.mapgen_threads_mtx);
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

// prepare spawn region
void server_map_prepare_spawn()
{
	s32 done = 0;
	s32 dist = server->config.simulation_distance;
	s32 total = (dist * 2 + 1);
	total *= total * total;
	s32 last_percentage = -1;

	for (s32 x = -dist; x <= (s32) dist; x++) {
		for (s32 y = -dist; y <= (s32) dist; y++) {
			for (s32 z = -dist; z <= (s32) dist; z++) {
				if (interrupted) {
					join_mapgen_threads();
					return;
				}

				MapBlock *block = map_get_block(server_map.map, (v3s32) {x, y, z}, true);

				pthread_mutex_lock(&block->mtx);
				if (((MapBlockExtraData *) block->extra)->state == MBS_CREATED)
					launch_mapgen_thread(block);
				pthread_mutex_unlock(&block->mtx);

				done++;

				s32 percentage = 100.0 * done / total;

				if (percentage > last_percentage) {
					last_percentage = percentage;
					printf("Preparing spawn... %d%%\n", percentage);
				}
			}
		}
	}

	join_mapgen_threads();

	s64 saved_spawn_height;
	if (database_load_meta("spawn_height", &saved_spawn_height)) {
		server_map.spawn_height = saved_spawn_height;
	} else {
		s32 spawn_height = -1;

		while (map_get_node(server_map.map, (v3s32) {0, ++spawn_height, 0}).type != NODE_AIR)
			;

		server_map.spawn_height = spawn_height + 5;
		generate_spawn_hut();
		database_save_meta("spawn_height", server_map.spawn_height);
	}
}
