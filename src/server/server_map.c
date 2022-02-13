#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "interrupt.h"
#include "map.h"
#include "server/database.h"
#include "server/mapgen.h"
#include "server/server_config.h"
#include "server/server_map.h"
#include "util.h"

// this file is too long
struct ServerMap server_map;

// utility functions

// return true if a player is close enough to a block to access it
static bool within_simulation_distance(ServerPlayer *player, v3s32 blkp, u32 dist)
{
	pthread_rwlock_rdlock(&player->pos_lock);
	v3s32 ppos = map_node_to_block_pos((v3s32) {player->pos.x, player->pos.y, player->pos.z}, NULL);
	pthread_rwlock_unlock(&player->pos_lock);

	return abs(ppos.x - blkp.x) <= (s32) dist
		&& abs(ppos.y - blkp.y) <= (s32) dist
		&& abs(ppos.z - blkp.z) <= (s32) dist;
}

// send a block to a client and reset block request
static void send_block(ServerPlayer *player, MapBlock *block)
{
	if (! within_simulation_distance(player, block->pos, server_config.simulation_distance))
		return;

	dragonnet_peer_send_ToClientBlock(player->peer, &(ToClientBlock) {
		.pos = block->pos,
		.data = ((MapBlockExtraData *) block->extra)->data,
	});
}

// send block to near clients
// block mutex has to be locked
static void send_block_to_near(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	if (extra->state == MBS_GENERATING)
		return;

	Blob_free(&extra->data);
	extra->data = map_serialize_block(block);

	database_save_block(block);

	if (extra->state == MBS_CREATED)
		return;

	server_player_iterate((void *) &send_block, block);
}

// list_clear_func callback for sending changed blocks to near clients
static void list_send_block(void *key, unused void *value, unused void *arg)
{
	MapBlock *block = key;

	pthread_mutex_lock(&block->mtx);
	send_block_to_near(block);
	pthread_mutex_unlock(&block->mtx);
}

// me when the
static void mapgen_step()
{
	MapBlock *block = queue_dequeue(server_map.mapgen_tasks);

	if (! block)
		return;

	MapBlockExtraData *extra = block->extra;

	List changed_blocks = list_create(NULL);
	list_put(&changed_blocks, block, NULL);

	mapgen_generate_block(block, &changed_blocks);

	pthread_mutex_lock(&block->mtx);
	extra->state = MBS_READY;
	pthread_mutex_unlock(&block->mtx);

	list_clear_func(&changed_blocks, &list_send_block, NULL);

	pthread_mutex_lock(&server_map.num_blocks_mtx);
	server_map.num_blocks--;
	pthread_mutex_unlock(&server_map.num_blocks_mtx);
}

// there was a time when i wrote actually useful comments lol
static void *mapgen_thread(unused void *arg)
{
	while (! server_map.cancel)
		mapgen_step();

	return NULL;
}

// enqueue block
static void generate_block(MapBlock *block)
{
	if (server_map.cancel)
		return;

	pthread_mutex_lock(&server_map.num_blocks_mtx);
	server_map.num_blocks++;
	pthread_mutex_unlock(&server_map.num_blocks_mtx);

	MapBlockExtraData *extra = block->extra;
	extra->state = MBS_GENERATING;
	queue_enqueue(server_map.mapgen_tasks, block);
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
		extra->data = (Blob) {0, NULL};

		ITERATE_MAPBLOCK {
			block->data[x][y][z] = map_node_create(NODE_AIR, (Blob) {0, NULL});
			extra->mgsb.raw.nodes[x][y][z] = MGS_VOID;
		}
	}
}

// callback for deleting a block
// free extra data
static void on_delete_block(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	Blob_free(&extra->data);
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

	MapgenStage *old_mgs = &((MapBlockExtraData *) block->extra)->mgsb.raw.nodes[offset.x][offset.y][offset.z];

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

// generate a hut for new players to spawn in
static void generate_spawn_hut()
{
	Blob wood_color = {0, NULL};
	HSLData_write(&wood_color, &(HSLData) {{0.11f, 1.0f, 0.29f}});

	List changed_blocks = list_create(NULL);

	for (s32 x = -4; x <= +4; x++) {
		for (s32 y = 0; y <= 3; y++) {
			for (s32 z = -3; z <= +2; z++) {
				mapgen_set_node((v3s32) {x, server_map.spawn_height + y, z}, map_node_create(NODE_AIR, (Blob) {0, NULL}), MGS_PLAYER, &changed_blocks);
			}
		}
	}

	for (s32 x = -5; x <= +5; x++) {
		for (s32 z = -4; z <= +3; z++) {
			mapgen_set_node((v3s32) {x, server_map.spawn_height - 1, z}, map_node_create(NODE_OAK_WOOD, wood_color), MGS_PLAYER, &changed_blocks);
			mapgen_set_node((v3s32) {x, server_map.spawn_height + 4, z}, map_node_create(NODE_OAK_WOOD, wood_color), MGS_PLAYER, &changed_blocks);
		}
	}

	for (s32 y = 0; y <= 3; y++) {
		for (s32 x = -5; x <= +5; x++) {
			mapgen_set_node((v3s32) {x, server_map.spawn_height + y, -4}, map_node_create(((y == 1 || y == 2) && ((x >= -3 && x <= -1) || (x >= +1 && x <= +2))) ? NODE_AIR : NODE_OAK_WOOD, wood_color), MGS_PLAYER, &changed_blocks);
			mapgen_set_node((v3s32) {x, server_map.spawn_height + y, +3}, map_node_create(((y == 1 || y == 2) && ((x >= -3 && x <= -2) || (x >= +1 && x <= +3))) ? NODE_AIR : NODE_OAK_WOOD, wood_color), MGS_PLAYER, &changed_blocks);
		}
	}

	for (s32 y = 0; y <= 3; y++) {
		for (s32 z = -3; z <= +2; z++) {
			mapgen_set_node((v3s32) {-5, server_map.spawn_height + y, z}, map_node_create(NODE_OAK_WOOD, wood_color), MGS_PLAYER, &changed_blocks);
			mapgen_set_node((v3s32) {+5, server_map.spawn_height + y, z}, map_node_create(((y != 3) && (z == -1 || z == +0)) ? NODE_AIR : NODE_OAK_WOOD, wood_color), MGS_PLAYER, &changed_blocks);
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

			mapgen_set_node(pos, map_node_create(node == NODE_LAVA ? NODE_VULCANO_STONE : NODE_OAK_WOOD, wood_color), MGS_PLAYER, &changed_blocks);
		}
	}

	list_clear_func(&changed_blocks, &list_send_block, NULL);
}

// public functions

// ServerMap singleton constructor
void server_map_init()
{
	server_map.map = map_create((MapCallbacks) {
		.create_block = &on_create_block,
		.delete_block = &on_delete_block,
		.get_block = &on_get_block,
		.set_node = &on_set_node,
		.after_set_node = &on_after_set_node,
	});

	server_map.cancel = false;
	server_map.mapgen_tasks = queue_create();
	server_map.mapgen_threads = malloc(sizeof *server_map.mapgen_threads * server_config.mapgen_threads);
	server_map.num_blocks = 0;
	pthread_mutex_init(&server_map.num_blocks_mtx, NULL);

	for (unsigned int i = 0; i < server_config.mapgen_threads; i++)
		pthread_create(&server_map.mapgen_threads[i], NULL, &mapgen_thread, NULL);
}

// ServerMap singleton destructor
void server_map_deinit()
{
	queue_finish(server_map.mapgen_tasks);
	server_map.cancel = true;
	queue_cancel(server_map.mapgen_tasks);

	for (unsigned int i = 0; i < server_config.mapgen_threads; i++)
		pthread_join(server_map.mapgen_threads[i], NULL);
	free(server_map.mapgen_threads);

	pthread_mutex_destroy(&server_map.num_blocks_mtx);
	queue_delete(server_map.mapgen_tasks);
	map_delete(server_map.map);
}

// handle block request from client (thread safe)
void server_map_requested_block(ServerPlayer *player, v3s32 pos)
{
	if (within_simulation_distance(player, pos, server_config.simulation_distance)) {
		MapBlock *block = map_get_block(server_map.map, pos, true);

		pthread_mutex_lock(&block->mtx);

		MapBlockExtraData *extra = block->extra;
		switch (extra->state) {
			case MBS_CREATED:
				generate_block(block);
				break;

			case MBS_GENERATING:
				break;

			case MBS_READY:
				send_block(player, block);
		};

		pthread_mutex_unlock(&block->mtx);
	}
}

static void update_percentage()
{
	static s32 total = 3 * 3 * 21;
	static s32 done = -1;
	static s32 last_percentage = -1;

	if (done < total)
		done++;

	pthread_mutex_lock(&server_map.num_blocks_mtx);
	s32 percentage = 100.0 * (done - server_map.num_blocks) / total;
	pthread_mutex_unlock(&server_map.num_blocks_mtx);

	if (percentage > last_percentage) {
		last_percentage = percentage;
		printf("Preparing spawn... %d%%\n", percentage);
	}

}

// prepare spawn region
void server_map_prepare_spawn()
{
	update_percentage();

	for (s32 x = -1; x <= (s32) 1; x++) {
		for (s32 y = -10; y <= (s32) 10; y++) {
			for (s32 z = -1; z <= (s32) 1; z++) {
				if (interrupt->done)
					return;

				MapBlock *block = map_get_block(server_map.map, (v3s32) {x, y, z}, true);

				pthread_mutex_lock(&block->mtx);
				if (((MapBlockExtraData *) block->extra)->state == MBS_CREATED)
					generate_block(block);
				pthread_mutex_unlock(&block->mtx);

				update_percentage();
			}
		}
	}

	while (true) {
		pthread_mutex_lock(&server_map.num_blocks_mtx);
		bool done = (server_map.num_blocks == 0);
		pthread_mutex_unlock(&server_map.num_blocks_mtx);

		if (done)
			break;

		update_percentage();
		sched_yield();
	}

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
