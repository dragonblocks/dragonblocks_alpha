#include <stdio.h>
#include <stdlib.h>
#include "client/blockmesh.h"
#include "client/facecache.h"
#include "client/client_map.h"
#include "client/client_player.h"
#include "client/debug_menu.h"
#include "util.h"
#define MAX_BLOCK_REQUESTS 4

struct ClientMap client_map;
Client *client;

// meshgen functions

// dequeue callback to thread-safely update
static void set_dequeued(void *arg)
{
	MapBlock *block = arg;

	pthread_mutex_lock(&block->mtx);
	((MapBlockExtraData *) block->extra)->queue = false;
	pthread_mutex_unlock(&block->mtx);
}

// mesh generator step
static void meshgen_step()
{
	MapBlock *block;

	if ((block = queue_dequeue_callback(client_map.queue, &set_dequeued)))
		blockmesh_make(block);
	else
		sched_yield();
}

// pthread start routine for meshgen thread
static void *meshgen_thread(unused void *arg)
{
	while (! client_map.cancel)
		meshgen_step();

	return NULL;
}

// sync functions

// send block request command to server
static void request_position(v3s32 pos)
{
	pthread_mutex_lock(&client->mtx);
	(void) (write_u32(client->fd, SC_REQUEST_BLOCK) && write_v3s32(client->fd, pos));
	pthread_mutex_unlock(&client->mtx);
}

// mapblock synchronisation step
static void sync_step()
{
	static u64 tick = 0;
	static v3s32 *old_requested_positions = NULL;
	static size_t old_requested_positions_count = 0;

	u64 last_tick = tick++;

	v3f64 player_pos = client_player_get_position();
	v3s32 center = map_node_to_block_pos((v3s32) {player_pos.x, player_pos.y, player_pos.z}, NULL);

	v3s32 *requested_positions = malloc(sizeof(v3s32) * MAX_BLOCK_REQUESTS);
	size_t requested_positions_count = 0;

	for (size_t i = 0; i < client_map.blocks_count; i++) {
		v3s32 pos = facecache_face(i, &center);
		MapBlock *block = map_get_block(client_map.map, pos, false);

		if (block) {
			pthread_mutex_lock(&block->mtx);
			MapBlockExtraData *extra = block->extra;

			switch (extra->state) {
				case MBS_READY:
					if (extra->last_synced < last_tick)
						request_position(pos);
					fallthrough;

				case MBS_FRESH:
					extra->state = MBS_READY;
					extra->last_synced = tick;
					break;

				case MBS_RECIEVING:
					break;
			}
			pthread_mutex_unlock(&block->mtx);
		} else if (requested_positions_count < MAX_BLOCK_REQUESTS) {
			bool should_request = true;

			for (size_t i = 0; i < old_requested_positions_count; i++) {
				if (v3s32_equals(old_requested_positions[i], pos)) {
					should_request = false;
					break;
				}
			}

			if (should_request)
				request_position(pos);

			requested_positions[requested_positions_count++] = pos;
		}
	}

	if (old_requested_positions)
		free(old_requested_positions);

	old_requested_positions = requested_positions;
	old_requested_positions_count = requested_positions_count;
}

// pthread start routine for sync thread
static void *sync_thread(unused void *arg)
{
	while (! client_map.cancel)
		sync_step();

	return NULL;
}

// map callbacks
// note: all these functions require the block mutex to be locked, which is always the case when a map callback is invoked

// callback for initializing a newly created block
// allocate and initialize extra data
static void on_create_block(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra = malloc(sizeof(MapBlockExtraData));

	extra->state = MBS_RECIEVING;
	extra->queue = false;
	extra->last_synced = 0;
	extra->obj = NULL;
}

// callback for deleting a block
// free extra data
static void on_delete_block(MapBlock *block)
{
	free(block->extra);
}

// callback for determining whether a block should be returned by map_get_block
// hold back blocks that have not been fully read from server yet when the create flag is set to true
static bool on_get_block(MapBlock *block, bool create)
{
	return create || ((MapBlockExtraData *) block->extra)->state > MBS_RECIEVING;
}

// public functions

// ClientMap singleton constructor
void client_map_init(Client *cli)
{
	client = cli;

	client_map.map = map_create((MapCallbacks) {
		.create_block = &on_create_block,
		.delete_block = &on_delete_block,
		.get_block = &on_get_block,
		.set_node = NULL,
		.after_set_node = NULL,
	});
	client_map.queue = queue_create();
	client_map.cancel = false;
	client_map.sync_thread = 0;
	client_map_set_simulation_distance(10);

	for (int i = 0; i < NUM_MESHGEN_THREADS; i++)
		client_map.meshgen_threads[i] = 0;
}

// ClientMap singleton destructor
void client_map_deinit()
{
	queue_delete(client_map.queue);
	map_delete(client_map.map);
}

// start meshgen and sync threads
void client_map_start()
{
	for (int i = 0; i < NUM_MESHGEN_THREADS; i++)
		pthread_create(&client_map.meshgen_threads[i], NULL, &meshgen_thread, NULL);

	pthread_create(&client_map.sync_thread, NULL, &sync_thread, NULL);
}

// stop meshgen and sync threads
void client_map_stop()
{
	client_map.cancel = true;

	for (int i = 0; i < NUM_MESHGEN_THREADS; i++)
		if (client_map.meshgen_threads[i])
			pthread_join(client_map.meshgen_threads[i], NULL);

	if (client_map.sync_thread)
		pthread_join(client_map.sync_thread, NULL);
}

// update simulation distance
void client_map_set_simulation_distance(u32 simulation_distance)
{
	client_map.simulation_distance = simulation_distance;
	client_map.blocks_count = facecache_count(simulation_distance);
}

// called when a block was actually recieved from server
void client_map_block_received(MapBlock *block)
{
	pthread_mutex_lock(&block->mtx);
	MapBlockExtraData *extra = block->extra;
	if (extra->state == MBS_RECIEVING)
		extra->state = MBS_FRESH;
	pthread_mutex_unlock(&block->mtx);

	client_map_schedule_update_block_mesh(block);

	client_map_schedule_update_block_mesh(map_get_block(client_map.map, (v3s32) {block->pos.x + 1, block->pos.y + 0, block->pos.z + 0}, false));
	client_map_schedule_update_block_mesh(map_get_block(client_map.map, (v3s32) {block->pos.x + 0, block->pos.y + 1, block->pos.z + 0}, false));
	client_map_schedule_update_block_mesh(map_get_block(client_map.map, (v3s32) {block->pos.x + 0, block->pos.y + 0, block->pos.z + 1}, false));
	client_map_schedule_update_block_mesh(map_get_block(client_map.map, (v3s32) {block->pos.x - 1, block->pos.y - 0, block->pos.z - 0}, false));
	client_map_schedule_update_block_mesh(map_get_block(client_map.map, (v3s32) {block->pos.x - 0, block->pos.y - 1, block->pos.z - 0}, false));
	client_map_schedule_update_block_mesh(map_get_block(client_map.map, (v3s32) {block->pos.x - 0, block->pos.y - 0, block->pos.z - 1}, false));
}

// enqueue block to mesh update queue
void client_map_schedule_update_block_mesh(MapBlock *block)
{
	if (! block)
		return;

	pthread_mutex_lock(&block->mtx);
	MapBlockExtraData *extra = block->extra;
	if (! extra->queue) {
		extra->queue = true;
		queue_enqueue(client_map.queue, block);
	}
	pthread_mutex_unlock(&block->mtx);
}
