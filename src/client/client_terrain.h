#ifndef _CLIENT_TERRAIN_H_
#define _CLIENT_TERRAIN_H_

#include <stdatomic.h>
#include <stdbool.h>
#include "client/model.h"
#include "terrain.h"
#include "types.h"

#define CHUNK_MODE_NOCREATE 2

typedef enum {
	CHUNK_STATE_INIT,
	CHUNK_STATE_RECV,
	CHUNK_STATE_DEPS,
	CHUNK_STATE_DIRTY,
	CHUNK_STATE_CLEAN,
} TerrainChunkState;

typedef struct {
	bool queue;
	TerrainChunkState state;
	pthread_rwlock_t lock_state;

	// accessed only by recv thread
	TerrainChunk *neighbors[6];
	unsigned int num_neighbors;

	// write is protected by mtx_model, read is atomic
	atomic_bool depends[6];

	bool has_model;
	Model *model;
	pthread_mutex_t mtx_model;

	// protected by chunk data lock
	bool empty;

	// accessed only by sync thread
	u64 sync;
} TerrainChunkMeta;

extern Terrain *client_terrain;

void client_terrain_init();                                          // called on startup
void client_terrain_deinit();                                        // called on shutdown
void client_terrain_set_load_distance(u32 dist);                     // update load distance
u32 client_terrain_get_load_distance();                              // return load distance
void client_terrain_start();                                         // start meshgen and sync threads
void client_terrain_stop();                                          // stop meshgen and sync threads
void client_terrain_meshgen_task(TerrainChunk *chunk, bool changed); // enqueue chunk to mesh update queue
void client_terrain_receive_chunk(void *peer, ToClientChunk *pkt);   // callback to deserialize chunk from network

#endif
