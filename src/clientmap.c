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

static void *meshgen_thread(void *unused)
{
	(void) unused;

	while (! meshgen.cancel) {
		MapBlock *block;
		if ((block = dequeue_callback(meshgen.queue, &set_block_ready)))
			make_block_mesh(block, client->scene);
		else
			sched_yield();
	}

	return NULL;
}

void clientmap_init(Client *cli)
{
	client = cli;
	meshgen.queue = create_queue();
	pthread_create(&meshgen.thread, NULL, &meshgen_thread, NULL);
}

void clientmap_deinit()
{
	meshgen.cancel = true;
	pthread_join(meshgen.thread, NULL);
}

void clientmap_block_changed(MapBlock *block)
{
	pthread_mutex_lock(&block->mtx);
	if (block->state == MBS_MODIFIED) {
		block->state = MBS_PROCESSING;
		enqueue(meshgen.queue, block);
	}
	pthread_mutex_unlock(&block->mtx);
}
