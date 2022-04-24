#define _GNU_SOURCE // don't worry, GNU extensions are only used when available
#include <assert.h>
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
#include "server/server_node.h"
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
	v3s32 ppos = terrain_chunkp((v3s32) {player->pos.x, player->pos.y, player->pos.z});
	pthread_rwlock_unlock(&player->lock_pos);

	return abs(ppos.x - cpos.x) <= (s32) dist
		&& abs(ppos.y - cpos.y) <= (s32) dist
		&& abs(ppos.z - cpos.z) <= (s32) dist;
}

// send a chunk to a client and reset chunk request
static void send_chunk_to_client(ServerPlayer *player, TerrainChunk *chunk)
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

	pthread_mutex_lock(&meta->mtx);
	meta->state = CHUNK_STATE_READY;
	pthread_mutex_unlock(&meta->mtx);

	server_terrain_lock_and_send_chunks(&changed_chunks);

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

	meta->state = CHUNK_STATE_GENERATING;
	queue_enq(&terrain_gen_tasks, chunk);
}

// callback for initializing a newly created chunk
// load chunk from database or initialize state, tgstage buffer and data
static void on_create_chunk(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra = malloc(sizeof *meta);
	pthread_mutex_init(&meta->mtx, NULL);

	if (database_load_chunk(chunk)) {
		meta->data = terrain_serialize_chunk(server_terrain, chunk, &server_node_serialize_client);
	} else {
		meta->state = CHUNK_STATE_CREATED;
		meta->data = (Blob) {0, NULL};

		CHUNK_ITERATE {
			chunk->data[x][y][z] = server_node_create(NODE_AIR);
			meta->tgsb.raw.nodes[x][y][z] = STAGE_VOID;
		}
	}
}

// callback for deleting a chunk
// free meta data
static void on_delete_chunk(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;
	pthread_mutex_destroy(&meta->mtx);

	Blob_free(&meta->data);
	free(meta);
}

// callback for determining whether a chunk should be returned by terrain_get_chunk
// hold back chunks that are not fully generated except when the create flag is set to true
static bool on_get_chunk(TerrainChunk *chunk, int mode)
{
	if (mode == CHUNK_MODE_CREATE)
		return true;

	TerrainChunkMeta *meta = chunk->extra;
	pthread_mutex_lock(&meta->mtx);

	bool ret = meta->state == CHUNK_STATE_READY;

	pthread_mutex_unlock(&meta->mtx);
	return ret;
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

	v3f32 wood_color = {
		(f32) 0x7d / 0xff,
		(f32) 0x54 / 0xff,
		(f32) 0x35 / 0xff};

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

			server_terrain_gen_node(pos, node == NODE_LAVA
					? server_node_create(NODE_VULCANO_STONE)
					: server_node_create_color(NODE_OAK_WOOD, wood_color),
				STAGE_PLAYER, &changed_chunks);
		}
	}

	server_terrain_lock_and_send_chunks(&changed_chunks);
}

// public functions

// called on server startup
void server_terrain_init()
{
	server_terrain = terrain_create();
	server_terrain->callbacks.create_chunk   = &on_create_chunk;
	server_terrain->callbacks.delete_chunk   = &on_delete_chunk;
	server_terrain->callbacks.get_chunk      = &on_get_chunk;
	server_terrain->callbacks.delete_node    = &server_node_delete;

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
		TerrainChunk *chunk = terrain_get_chunk(server_terrain, pos, CHUNK_MODE_CREATE);
		TerrainChunkMeta *meta = chunk->extra;

		pthread_mutex_lock(&meta->mtx);
		switch (meta->state) {
			case CHUNK_STATE_CREATED:
				generate_chunk(chunk);
				break;

			case CHUNK_STATE_GENERATING:
				break;

			case CHUNK_STATE_READY:
				send_chunk_to_client(player, chunk);
				break;
		};

		pthread_mutex_unlock(&meta->mtx);
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

				TerrainChunk *chunk = terrain_get_chunk(server_terrain, (v3s32) {x, y, z}, CHUNK_MODE_CREATE);
				TerrainChunkMeta *meta = chunk->extra;

				pthread_mutex_lock(&meta->mtx);
				if (meta->state == CHUNK_STATE_CREATED)
					generate_chunk(chunk);
				pthread_mutex_unlock(&meta->mtx);

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

void server_terrain_gen_node(v3s32 pos, TerrainNode node, TerrainGenStage new_tgs, List *changed_chunks)
{
	v3s32 offset;
	TerrainChunk *chunk = terrain_get_chunk_nodep(server_terrain, pos, &offset, CHUNK_MODE_CREATE);
	TerrainChunkMeta *meta = chunk->extra;

	assert(pthread_rwlock_wrlock(&chunk->lock) == 0);

	u32 *tgs = &meta->tgsb.raw.nodes[offset.x][offset.y][offset.z];

	if (new_tgs < *tgs) {
		pthread_rwlock_unlock(&chunk->lock);
		server_node_delete(&node);
		return;
	}

	*tgs = new_tgs;
	chunk->data[offset.x][offset.y][offset.z] = node;

	if (changed_chunks)
		list_add(changed_chunks, chunk, chunk, &cmp_ref, NULL);
	else
		server_terrain_send_chunk(chunk);

	pthread_rwlock_unlock(&chunk->lock);
}

s32 server_terrain_spawn_height()
{
	// wow, so useful!
	return spawn_height;
}

// send chunk to near clients
// meta mutex has to be locked
void server_terrain_send_chunk(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;

	if (meta->state == CHUNK_STATE_GENERATING)
		return;

	assert(pthread_rwlock_rdlock(&chunk->lock) == 0);

	Blob_free(&meta->data);
	meta->data = terrain_serialize_chunk(server_terrain, chunk, &server_node_serialize_client);
	database_save_chunk(chunk);

	pthread_rwlock_unlock(&chunk->lock);

	if (meta->state == CHUNK_STATE_CREATED)
		return;

	server_player_iterate(&send_chunk_to_client, chunk);
}

void server_terrain_lock_and_send_chunk(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;

	pthread_mutex_lock(&meta->mtx);
	server_terrain_send_chunk(chunk);
	pthread_mutex_unlock(&meta->mtx);
}

void server_terrain_lock_and_send_chunks(List *changed_chunks)
{
	list_clr(changed_chunks, &server_terrain_lock_and_send_chunk, NULL, NULL);
}
