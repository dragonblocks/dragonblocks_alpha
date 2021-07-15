#include <stdlib.h>
#include <pthread.h>
#include "client/blockmesh.h"
#include "client/client_map.h"
#include "queue.h"

static struct
{
	Map *map;
	Queue *queue;
	pthread_t thread;
	bool cancel;
} client_map;

static void set_block_ready(void *block)
{
	((MapBlock *) block)->state = MBS_READY;
}

static void *meshgen_thread(__attribute__((unused)) void *unused)
{
	while (! client_map.cancel) {
		MapBlock *block;
		if ((block = queue_dequeue_callback(client_map.queue, &set_block_ready)))
			blockmesh_make(block, client_map.map);
		else
			sched_yield();
	}

	return NULL;
}

void client_map_init(Map *map)
{
	client_map.map = map;
	client_map.queue = queue_create();
}

void client_map_start_meshgen()
{
	pthread_create(&client_map.thread, NULL, &meshgen_thread, NULL);
}

void client_map_deinit()
{
	client_map.cancel = true;
	queue_delete(client_map.queue);
	if (client_map.thread)
		pthread_join(client_map.thread, NULL);
}

static void schedule_update_block(MapBlock *block)
{
	if (! block)
		return;

	pthread_mutex_lock(&block->mtx);
	if (block->state != MBS_PROCESSING) {
		block->state = MBS_PROCESSING;
		queue_enqueue(client_map.queue, block);
	}
	pthread_mutex_unlock(&block->mtx);
}

void client_map_block_changed(MapBlock *block)
{
	schedule_update_block(block);

	schedule_update_block(map_get_block(client_map.map, (v3s32) {block->pos.x + 1, block->pos.y + 0, block->pos.z + 0}, false));
	schedule_update_block(map_get_block(client_map.map, (v3s32) {block->pos.x + 0, block->pos.y + 1, block->pos.z + 0}, false));
	schedule_update_block(map_get_block(client_map.map, (v3s32) {block->pos.x + 0, block->pos.y + 0, block->pos.z + 1}, false));
	schedule_update_block(map_get_block(client_map.map, (v3s32) {block->pos.x - 1, block->pos.y - 0, block->pos.z - 0}, false));
	schedule_update_block(map_get_block(client_map.map, (v3s32) {block->pos.x - 0, block->pos.y - 1, block->pos.z - 0}, false));
	schedule_update_block(map_get_block(client_map.map, (v3s32) {block->pos.x - 0, block->pos.y - 0, block->pos.z - 1}, false));
}
