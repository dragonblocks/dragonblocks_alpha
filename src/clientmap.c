#include <stdlib.h>
#include "clientmap.h"
#include "blockmesh.h"
#include "queue.h"

static struct
{
	Queue *queue;
	pthread_t thread;
	bool cancel;
} meshgen;

static Client *client = NULL;

static void set_block_ready(void *block)
{
	((MapBlock *) block)->state = MBS_READY;
}

static void *meshgen_thread(__attribute__((unused)) void *unused)
{
	while (! meshgen.cancel) {
		MapBlock *block;
		if ((block = dequeue_callback(meshgen.queue, &set_block_ready)))
			make_block_mesh(block, client->map, client->scene);
		else
			sched_yield();
	}

	return NULL;
}

void clientmap_init(Client *cli)
{
	client = cli;
	meshgen.queue = create_queue();
}

void clientmap_start_meshgen()
{
	pthread_create(&meshgen.thread, NULL, &meshgen_thread, NULL);
}

void clientmap_deinit()
{
	meshgen.cancel = true;
	delete_queue(meshgen.queue);
	if (meshgen.thread)
		pthread_join(meshgen.thread, NULL);
}

static void schedule_update_block(MapBlock *block)
{
	if (! block)
		return;

	pthread_mutex_lock(&block->mtx);
	if (block->state != MBS_PROCESSING) {
		block->state = MBS_PROCESSING;
		enqueue(meshgen.queue, block);
	}
	pthread_mutex_unlock(&block->mtx);
}

void clientmap_block_changed(MapBlock *block)
{
	schedule_update_block(block);

	schedule_update_block(map_get_block(client->map, (v3s32) {block->pos.x + 1, block->pos.y + 0, block->pos.z + 0}, false));
	schedule_update_block(map_get_block(client->map, (v3s32) {block->pos.x + 0, block->pos.y + 1, block->pos.z + 0}, false));
	schedule_update_block(map_get_block(client->map, (v3s32) {block->pos.x + 0, block->pos.y + 0, block->pos.z + 1}, false));
	schedule_update_block(map_get_block(client->map, (v3s32) {block->pos.x - 1, block->pos.y - 0, block->pos.z - 0}, false));
	schedule_update_block(map_get_block(client->map, (v3s32) {block->pos.x - 0, block->pos.y - 1, block->pos.z - 0}, false));
	schedule_update_block(map_get_block(client->map, (v3s32) {block->pos.x - 0, block->pos.y - 0, block->pos.z - 1}, false));
}
