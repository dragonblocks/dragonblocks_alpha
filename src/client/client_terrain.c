#define _GNU_SOURCE // don't worry, GNU extensions are only used when available
#include <assert.h>
#include <dragonstd/queue.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "client/client.h"
#include "client/facecache.h"
#include "client/client_config.h"
#include "client/client_node.h"
#include "client/client_player.h"
#include "client/client_terrain.h"
#include "client/debug_menu.h"
#include "client/terrain_gfx.h"
#include "facedir.h"

#define MAX_REQUESTS 4

Terrain *client_terrain;

static atomic_bool cancel;         // used to notify meshgen and sync thread about quit
static Queue meshgen_tasks;        // TerrainCHunk * queue (thread safe)
static pthread_t *meshgen_threads; // consumer threads for meshgen queue
static pthread_t sync_thread;      // this thread requests new / changed chunks from server
static u32 load_distance;          // load distance sent by server
static size_t load_chunks;         // cached number of facecache positions to process every sync step (matches load distance)

// meshgen functions

// dequeue callback to update queue state in a thread safe manner
static TerrainChunk *set_dequeued(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra;

	assert(pthread_rwlock_wrlock(&meta->lock_state) == 0);
	meta->queue = false;
	if (meta->state < CHUNK_STATE_DIRTY)
		chunk = NULL;
	else
		meta->state = CHUNK_STATE_CLEAN;
	pthread_rwlock_unlock(&meta->lock_state);

	return chunk;
}

// mesh generator step
static void meshgen_step()
{
	TerrainChunk *chunk = queue_deq(&meshgen_tasks, &set_dequeued);

	if (chunk)
		terrain_gfx_make_chunk_model(chunk);
}

// sync functions

// send chunk request command to server
static void request_chunk(v3s32 pos)
{
	dragonnet_peer_send_ToServerRequestChunk(client, &(ToServerRequestChunk) {
		.pos = pos
	});
}

// terrain synchronisation step
static void sync_step()
{
	static u64 tick = 1;
	static v3s32 *old_requests = NULL;
	static size_t old_num_requests = 0;

	ClientEntity *entity = client_player_entity_local();
	if (!entity) {
		sched_yield();
		return;
	}

	pthread_rwlock_rdlock(&entity->lock_pos_rot);
	v3s32 center = terrain_chunkp(v3f64_to_s32(entity->data.pos));
	pthread_rwlock_unlock(&entity->lock_pos_rot);

	refcount_drp(&entity->rc);

	u64 last_tick = tick++;

	v3s32 *requests = malloc(MAX_REQUESTS * sizeof *requests);
	size_t num_requests = 0;

	for (size_t i = 0; i < load_chunks; i++) {
		v3s32 pos = v3s32_add(facecache_get(i), center);
		TerrainChunk *chunk = terrain_get_chunk(client_terrain, pos, CHUNK_MODE_NOCREATE);

		if (chunk) {
			TerrainChunkMeta *meta = chunk->extra;

			// re-request chunks that got out of and then back into range
			if (meta->sync && meta->sync < last_tick)
				request_chunk(pos);

			meta->sync = tick;
		} else if (num_requests < MAX_REQUESTS) {
			// avoid duplicate requests
			bool requested = false;

			for (size_t i = 0; i < old_num_requests; i++) {
				if (v3s32_equals(old_requests[i], pos)) {
					requested = true;
					break;
				}
			}

			if (!requested)
				request_chunk(pos);

			requests[num_requests++] = pos;
		}
	}

	if (old_requests)
		free(old_requests);

	old_requests = requests;
	old_num_requests = num_requests;
}

// pthread routine for meshgen and sync thread

static struct LoopThread {
	const char *name;
	void (*step)();
} loop_threads[2] = {
	{"meshgen", &meshgen_step},
	{   "sync",    &sync_step},
};

static void *loop_routine(struct LoopThread *thread)
{
#ifdef __GLIBC__ // check whether bloat is enabled
	pthread_setname_np(pthread_self(), thread->name);
#endif // __GLIBC__

	// warning: extremely advanced logic
	while (!cancel)
		thread->step();

	return NULL;
}

// terrain callbacks
// note: all these functions require the chunk mutex to be locked, which is always the case when a terrain callback is invoked

// callback for initializing a newly created chunk
// allocate and initialize meta data
static void on_create_chunk(TerrainChunk *chunk)
{
	TerrainChunkMeta *meta = chunk->extra = malloc(sizeof *meta);

	meta->queue = false;
	meta->state = CHUNK_STATE_INIT;
	pthread_rwlock_init(&meta->lock_state, NULL);

	for (int i = 0; i < 6; i++)
		meta->neighbors[i] = 0;
	meta->num_neighbors = 0;

	for (int i = 0; i < 6; i++)
		meta->depends[i] = true;

	meta->has_model = false;
	meta->model = NULL;
	pthread_mutex_init(&meta->mtx_model, NULL);

	meta->empty = false;

	meta->sync = 0;
}

// callback for deleting a chunk
// free meta data
static void on_delete_chunk(TerrainChunk *chunk)
{
	free(chunk->extra);
}

// callback for determining whether a chunk should be returned by terrain_get_chunk
// hold back chunks that have not been fully read from server yet when the create flag is not set
static bool on_get_chunk(TerrainChunk *chunk, int mode)
{
	if (mode != CHUNK_MODE_PASSIVE)
		return true;

	TerrainChunkMeta *meta = chunk->extra;
	assert(pthread_rwlock_rdlock(&meta->lock_state) == 0);
	bool ret = meta->state > CHUNK_STATE_DEPS;
	pthread_rwlock_unlock(&meta->lock_state);

	return ret;
}

// public functions

// called on startup
void client_terrain_init()
{
	client_terrain = terrain_create();
	client_terrain->callbacks.create_chunk = &on_create_chunk;
	client_terrain->callbacks.delete_chunk = &on_delete_chunk;
	client_terrain->callbacks.get_chunk    = &on_get_chunk;
	client_terrain->callbacks.delete_node  = &client_node_delete;

	cancel = false;
	queue_ini(&meshgen_tasks);

	client_terrain_set_load_distance(10); // some initial fuck idk just in case server is stupid

	sync_thread = 0;
	meshgen_threads = malloc(sizeof *meshgen_threads * client_config.meshgen_threads);
	for (unsigned int i = 0; i < client_config.meshgen_threads; i++)
		meshgen_threads[i] = 0; // but why???
}

// called on shutdown
void client_terrain_deinit()
{
	queue_clr(&meshgen_tasks, NULL, NULL, NULL);
	terrain_delete(client_terrain);
}

// start meshgen and sync threads
void client_terrain_start()
{
	for (unsigned int i = 0; i < client_config.meshgen_threads; i++)
		pthread_create(&meshgen_threads[i], NULL, (void *) &loop_routine, &loop_threads[0]);

	pthread_create(&sync_thread, NULL, (void *) &loop_routine, &loop_threads[1]);
}

// stop meshgen and sync threads
void client_terrain_stop()
{
	cancel = true;
	queue_cnl(&meshgen_tasks);

	for (unsigned int i = 0; i < client_config.meshgen_threads; i++)
		if (meshgen_threads[i])
			pthread_join(meshgen_threads[i], NULL);
	free(meshgen_threads);

	if (sync_thread)
		pthread_join(sync_thread, NULL);
}

// update load distance
void client_terrain_set_load_distance(u32 dist)
{
	load_distance = dist;
	load_chunks = facecache_count(load_distance);
	debug_menu_changed(ENTRY_LOAD_DISTANCE);
}

// return load distance
u32 client_terrain_get_load_distance()
{
	return load_distance;
}

// enqueue chunk to mesh update queue
void client_terrain_meshgen_task(TerrainChunk *chunk, bool changed)
{
	TerrainChunkMeta *meta = chunk->extra;

	assert(pthread_rwlock_wrlock(&meta->lock_state) == 0);
	bool queue = meta->queue;
	pthread_rwlock_unlock(&meta->lock_state);

	if (queue)
		return;

	assert(pthread_rwlock_rdlock(&chunk->lock) == 0);
	bool empty = meta->empty;
	pthread_rwlock_unlock(&chunk->lock);

	if (empty)
		set_dequeued(chunk);

	pthread_mutex_lock(&meta->mtx_model);
	if (empty) {
		meta->has_model = true;

		if (meta->model) {
			meta->model->flags.delete = 1;
			meta->model = NULL;
		}
	} else {
		meta->queue = true;
		if (meta->has_model && changed)
			queue_ppd(&meshgen_tasks, chunk);
		else
			queue_enq(&meshgen_tasks, chunk);
	}
	pthread_mutex_unlock(&meta->mtx_model);
}

static void iterator_meshgen_task(TerrainChunk *chunk)
{
	client_terrain_meshgen_task(chunk, true);
}

void client_terrain_receive_chunk(__attribute__((unused)) void *peer, ToClientChunk *pkt)
{
	// get/create chunk
	TerrainChunk *chunk = terrain_get_chunk(client_terrain, pkt->pos, CHUNK_MODE_CREATE);
	TerrainChunkMeta *meta = chunk->extra;

	assert(pthread_rwlock_wrlock(&meta->lock_state) == 0);
	// remember whether this is the first time we're receiving the chunk
	bool init = meta->state == CHUNK_STATE_INIT;
	// change state to receiving
	meta->state = CHUNK_STATE_RECV;
	pthread_rwlock_unlock(&meta->lock_state);

	// notify/collect neighbors
	for (int i = 0; i < 6; i++) {
		// this is the reverse face index
		int j = i % 2 ? i - 1 : i + 1;

		if (init) {
			// if this is first time, initialize references in both ways

			// get existing neighbor chunk
			TerrainChunk *neighbor = terrain_get_chunk(
				client_terrain, v3s32_add(chunk->pos, facedir[i]), CHUNK_MODE_NOCREATE);
			if (!neighbor)
				continue;
			TerrainChunkMeta *neighbor_meta = neighbor->extra;

			// initialize reference from us to neighbor
			meta->neighbors[i] = neighbor;
			meta->num_neighbors++;

			// initialize reference from neighbor to us
			// they (obviously) don't have all neighbors yet, so they are already in RECV state
			neighbor_meta->neighbors[j] = chunk;
			neighbor_meta->num_neighbors++;
		} else {
			// get reference
			TerrainChunk *neighbor = meta->neighbors[i];
			if (!neighbor)
				continue;
			TerrainChunkMeta *neighbor_meta = neighbor->extra;

			// don't change state of non-dependant neighbor
			if (!neighbor_meta->depends[j])
				continue;

			// if neighbor depends on us, set them to deps resolval state
			assert(pthread_rwlock_wrlock(&neighbor_meta->lock_state) == 0);
			neighbor_meta->state = CHUNK_STATE_DEPS;
			pthread_rwlock_unlock(&neighbor_meta->lock_state);
		}
	}

	// deserialize data
	assert(pthread_rwlock_wrlock(&chunk->lock) == 0);
	meta->empty = (pkt->data.siz == 0);
	terrain_deserialize_chunk(client_terrain, chunk, pkt->data, &client_node_deserialize);
	pthread_rwlock_unlock(&chunk->lock);

	// collect meshgen tasks and schedule them after chunk states have been updated
	List meshgen_tasks;
	list_ini(&meshgen_tasks);

	// set own state to dirty (if all neighbors are there) or resolving deps else
	assert(pthread_rwlock_wrlock(&meta->lock_state) == 0);
	meta->state = meta->num_neighbors == 6 ? CHUNK_STATE_DIRTY : CHUNK_STATE_DEPS;
	pthread_rwlock_unlock(&meta->lock_state);

	// if all neighbors are there, schedule meshgen
	if (meta->num_neighbors == 6)
		list_apd(&meshgen_tasks, chunk);

	// notify neighbors (and self)
	for (int i = 0; i < 6; i++) {
		// select neighbor chunk
		TerrainChunk *neighbor = meta->neighbors[i];
		if (!neighbor)
			continue;
		TerrainChunkMeta *neighbor_meta = neighbor->extra;

		// don't bother with chunks that don't depend us
		if (!neighbor_meta->depends[i % 2 ? i - 1 : i + 1])
			continue;

		// don't change state if neighbors are not all present
		if (neighbor_meta->num_neighbors != 6)
			continue;

		// set state of dependant chunk to dirty
		assert(pthread_rwlock_wrlock(&neighbor_meta->lock_state) == 0);
		neighbor_meta->state = CHUNK_STATE_DIRTY;
		pthread_rwlock_unlock(&neighbor_meta->lock_state);

		// remeber to schedule meshgen task later
		list_apd(&meshgen_tasks, neighbor);
	}

	// schedule meshgen tasks
	list_clr(&meshgen_tasks, (void *) &iterator_meshgen_task, NULL, NULL);
}
