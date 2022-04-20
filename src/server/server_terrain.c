#define _GNU_SOURCE // don't worry, GNU extensions are only used when available
#include <dragonstd/queue.h>
#include <features.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "interrupt.h"
#include "server/database.h"
#include "server/schematic.h"
#include "server/server_config.h"
#include "server/server_terrain.h"
#include "server/terrain_gen.h"
#include "terrain.h"

// this file is too long
Terrain *server_terrain;

static atomic_bool cancel;                 // remove the smooth
static Queue terrain_gen_tasks;            // this is terry the fat shark
static pthread_t *terrain_gen_threads;     // thread pool
static s32 spawn_height;                   // elevation to spawn players at
static unsigned int num_gen_chunks;        // number of enqueued / generating chunks
static pthread_mutex_t mtx_num_gen_chunks; // lock to protect the above

// utility functions

// return true if a player is close enough to a chunk to access it
static bool within_load_distance(ServerPlayer *player, v3s32 cpos, u32 dist)
{
	pthread_rwlock_rdlock(&player->lock_pos);
	v3s32 ppos = terrain_node_to_chunk_pos((v3s32) {player->pos.x, player->pos.y, player->pos.z}, NULL);
	pthread_rwlock_unlock(&player->lock_pos);

	return abs(ppos.x - cpos.x) <= (s32) dist
		&& abs(ppos.y - cpos.y) <= (s32) dist
		&& abs(ppos.z - cpos.z) <= (s32) dist;
}

// send a chunk to a client and reset chunk request
static void send_chunk(ServerPlayer *player, TerrainChunk *chunk)
{
	if (!within_load_distance(player, chunk->pos, server_config.load_distance))
		return;

	pthread_rwlock_rdlock(&player->lock_peer);
	if (player->peer)
		dragonnet_peer_send_ToClientChunk(player->peer, &(ToClientChunk) {
			.pos = chunk->pos,
			.data = ((TerrainChunkMeta *) chunk->extra)->data,
		});
	pthread_rwlock_unlock(&player->lock_peer);
}

// send chunk to near clients
// chunk mutex has to be locked
static void send_chunk_to_near(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;

	if (meta->state == CHUNK_GENERATING)
		return;

	Blob_free(&meta->data);
	meta->data = terrain_serialize_chunk(chunk);

	database_save_chunk(chunk);

	if (meta->state == CHUNK_CREATED)
		return;

	server_player_iterate(&send_chunk, chunk);
}

// Iterator for sending changed chunks to near clients
static void iterator_send_chunk_to_near(TerrainChunk *chunk)
{
	pthread_mutex_lock(&chunk->mtx);
	send_chunk_to_near(chunk);
	pthread_mutex_unlock(&chunk->mtx);
}

// me when the
static void terrain_gen_step()
{
	// big chunkus
	TerrainChunk *chunk = queue_deq(&terrain_gen_tasks, NULL);

	if (!chunk)
		return;

	TerrainChunkMeta *meta = chunk->extra;

	List changed_chunks;
	list_ini(&changed_chunks);
	list_apd(&changed_chunks, chunk);

	terrain_gen_chunk(chunk, &changed_chunks);

	pthread_mutex_lock(&chunk->mtx);
	meta->state = CHUNK_READY;
	pthread_mutex_unlock(&chunk->mtx);

	list_clr(&changed_chunks, &iterator_send_chunk_to_near, NULL, NULL);

	pthread_mutex_lock(&mtx_num_gen_chunks);
	num_gen_chunks--;
	pthread_mutex_unlock(&mtx_num_gen_chunks);
}

// there was a time when i wrote actually useful comments lol
static void *terrain_gen_thread()
{
#ifdef __GLIBC__ // check whether bloat is enabled
	pthread_setname_np(pthread_self(), "terrain_gen");
#endif // __GLIBC__

	// extremely advanced logic
	while (!cancel)
		terrain_gen_step();

	return NULL;
}

// enqueue chunk
static void generate_chunk(TerrainChunk *chunk)
{
	if (cancel)
		return;

	pthread_mutex_lock(&mtx_num_gen_chunks);
	num_gen_chunks++;
	pthread_mutex_unlock(&mtx_num_gen_chunks);

	TerrainChunkMeta *meta = chunk->extra;
	meta->state = CHUNK_GENERATING;
	queue_enq(&terrain_gen_tasks, chunk);
}

// terrain callbacks
// note: all these functions require the chunk mutex to be locked, which is always the case when a terrain callback is invoked

// callback for initializing a newly created chunk
// load chunk from database or initialize state, tgstage buffer and data
static void on_create_chunk(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra = malloc(sizeof *meta);

	if (!database_load_chunk(chunk)) {
		meta->state = CHUNK_CREATED;
		meta->data = (Blob) {0, NULL};

		CHUNK_ITERATE {
			chunk->data[x][y][z] = terrain_node_create(NODE_AIR, (Blob) {0, NULL});
			meta->tgsb.raw.nodes[x][y][z] = STAGE_VOID;
		}
	}
}

// callback for deleting a chunk
// free meta data
static void on_delete_chunk(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;

	Blob_free(&meta->data);
	free(meta);
}

// callback for determining whether a chunk should be returned by terrain_get_chunk
// hold back chunks that are not fully generated except when the create flag is set to true
static bool on_get_chunk(TerrainChunk *chunk, bool create)
{
	TerrainChunkMeta *meta = chunk->extra;

	if (meta->state < CHUNK_READY && !create)
		return false;

	return true;
}

// callback for deciding whether a set_node call succeeds or not
// reject set_node calls that try to override nodes placed by later terraingen stages, else update tgs buffer - also make sure chunk is inserted into changed_chunks list
static bool on_set_node(TerrainChunk *chunk, v3u8 offset, __attribute__((unused)) TerrainNode *node, void *_arg)
{
	TerrainSetNodeArg *arg = _arg;

	TerrainGenStage new_tgs = arg ? arg->tgs : STAGE_PLAYER;
	TerrainGenStage *tgs = &((TerrainChunkMeta *) chunk->extra)->
		tgsb.raw.nodes[offset.x][offset.y][offset.z];

	if (new_tgs >= *tgs) {
		*tgs = new_tgs;

		if (arg)
			list_add(arg->changed_chunks, chunk, chunk, &cmp_ref, NULL);

		return true;
	}

	return false;
}

// callback for when chunk content changes
// send chunk to near clients if not part of terrain generation
static void on_after_set_node(TerrainChunk *chunk, __attribute__((unused)) v3u8 offset, void *arg)
{
	if (!arg)
		send_chunk_to_near(chunk);
}

// generate a hut for new players to spawn in
static void generate_spawn_hut()
{
	List changed_chunks;
	list_ini(&changed_chunks);

	List spawn_hut;
	schematic_load(&spawn_hut, RESSOURCE_PATH "schematics/spawn_hut.txt", (SchematicMapping[]) {
		{
			.color = {0x7d, 0x54, 0x35},
			.type = NODE_OAK_WOOD,
			.use_color = true,
		},
		{
			.color = {0x50, 0x37, 0x28},
			.type = NODE_OAK_WOOD,
			.use_color = true,
		},
	}, 2);

	schematic_place(&spawn_hut, (v3s32) {0, spawn_height, 0},
		STAGE_PLAYER, &changed_chunks);

	schematic_delete(&spawn_hut);

	// dynamic part of spawn hut - cannot be generated by a schematic

	v2s32 posts[6] = {
		{-4, -2},
		{-4, +3},
		{+3, -2},
		{+3, +3},
		{+4, +0},
		{+4, +1},
	};

	Blob wood_color = {0, NULL};
	ColorData_write(&wood_color, &(ColorData) {{(f32) 0x7d / 0xff, (f32) 0x54 / 0xff, (f32) 0x35 / 0xff}});

	for (int i = 0; i < 6; i++) {
		for (s32 y = spawn_height - 1;; y--) {
			v3s32 pos = {posts[i].x, y, posts[i].y};
			NodeType node = terrain_get_node(server_terrain, pos).type;

			if (i >= 4) {
				if (node != NODE_AIR)
					break;

				pos.y++;
			}

			if (node_def[node].solid)
				break;

			server_terrain_gen_node(pos,
				terrain_node_create(node == NODE_LAVA
						? NODE_VULCANO_STONE
						: NODE_OAK_WOOD,
					wood_color),
				STAGE_PLAYER, &changed_chunks);
		}
	}

	Blob_free(&wood_color);
	list_clr(&changed_chunks, &iterator_send_chunk_to_near, NULL, NULL);
}

// public functions

// called on server startup
void server_terrain_init()
{
	server_terrain = terrain_create();
	server_terrain->callbacks.create_chunk   = &on_create_chunk;
	server_terrain->callbacks.delete_chunk   = &on_delete_chunk;
	server_terrain->callbacks.get_chunk      = &on_get_chunk;
	server_terrain->callbacks.set_node       = &on_set_node;
	server_terrain->callbacks.after_set_node = &on_after_set_node;

	cancel = false;
	queue_ini(&terrain_gen_tasks);
	terrain_gen_threads = malloc(sizeof *terrain_gen_threads * server_config.terrain_gen_threads);
	num_gen_chunks = 0;
	pthread_mutex_init(&mtx_num_gen_chunks, NULL);

	for (unsigned int i = 0; i < server_config.terrain_gen_threads; i++)
		pthread_create(&terrain_gen_threads[i], NULL, (void *) &terrain_gen_thread, NULL);
}

// called on server shutdown
void server_terrain_deinit()
{
	queue_fin(&terrain_gen_tasks);
	cancel = true;
	queue_cnl(&terrain_gen_tasks);

	for (unsigned int i = 0; i < server_config.terrain_gen_threads; i++)
		pthread_join(terrain_gen_threads[i], NULL);
	free(terrain_gen_threads);

	pthread_mutex_destroy(&mtx_num_gen_chunks);
	queue_dst(&terrain_gen_tasks);
	terrain_delete(server_terrain);
}

// handle chunk request from client (thread safe)
void server_terrain_requested_chunk(ServerPlayer *player, v3s32 pos)
{
	if (within_load_distance(player, pos, server_config.load_distance)) {
		TerrainChunk *chunk = terrain_get_chunk(server_terrain, pos, true);

		pthread_mutex_lock(&chunk->mtx);

		TerrainChunkMeta *meta = chunk->extra;
		switch (meta->state) {
			case CHUNK_CREATED:
				generate_chunk(chunk);
				break;

			case CHUNK_GENERATING:
				break;

			case CHUNK_READY:
				send_chunk(player, chunk);
		};

		pthread_mutex_unlock(&chunk->mtx);
	}
}

static void update_percentage()
{
	static s32 total = 3 * 3 * 21;
	static s32 done = -1;
	static s32 last_percentage = -1;

	if (done < total)
		done++;

	pthread_mutex_lock(&mtx_num_gen_chunks);
	s32 percentage = 100.0 * (done - num_gen_chunks) / total;
	pthread_mutex_unlock(&mtx_num_gen_chunks);

	if (percentage > last_percentage) {
		last_percentage = percentage;
		printf("[verbose] preparing spawn... %d%%\n", percentage);
	}

}

// prepare spawn region
void server_terrain_prepare_spawn()
{
	update_percentage();

	for (s32 x = -1; x <= (s32) 1; x++) {
		for (s32 y = -10; y <= (s32) 10; y++) {
			for (s32 z = -1; z <= (s32) 1; z++) {
				if (interrupt.set)
					return;

				TerrainChunk *chunk = terrain_get_chunk(server_terrain, (v3s32) {x, y, z}, true);

				pthread_mutex_lock(&chunk->mtx);
				if (((TerrainChunkMeta *) chunk->extra)->state == CHUNK_CREATED)
					generate_chunk(chunk);
				pthread_mutex_unlock(&chunk->mtx);

				update_percentage();
			}
		}
	}

	for (;;) {
		update_percentage();

		pthread_mutex_lock(&mtx_num_gen_chunks);
		bool done = (num_gen_chunks == 0);
		pthread_mutex_unlock(&mtx_num_gen_chunks);

		if (done)
			break;

		sched_yield();
	}

	s64 saved_spawn_height;
	if (database_load_meta("spawn_height", &saved_spawn_height)) {
		spawn_height = saved_spawn_height;
	} else {
		spawn_height = -1;
		while (terrain_get_node(server_terrain, (v3s32) {0, ++spawn_height, 0}).type != NODE_AIR);
		spawn_height += 5;

		database_save_meta("spawn_height", spawn_height);
		generate_spawn_hut();
	}
}

void server_terrain_gen_node(v3s32 pos, TerrainNode node, TerrainGenStage tgs, List *changed_chunks)
{
	TerrainSetNodeArg arg = {
		.tgs = tgs,
		.changed_chunks = changed_chunks,
	};

	terrain_set_node(server_terrain, pos, node, true, &arg);
}

s32 server_terrain_spawn_height()
{
	// wow, so useful!
	return spawn_height;
}