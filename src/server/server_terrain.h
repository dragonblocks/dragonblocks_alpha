#ifndef _SERVER_TERRAIN_H_
#define _SERVER_TERRAIN_H_

#include <dragonstd/list.h>
#include <pthread.h>
#include "common/terrain.h"
#include "server/server_player.h"
#include "types.h"

typedef enum {
	CHUNK_STATE_CREATED,    // chunk exists but was not yet generated
	CHUNK_STATE_GENERATING, // currently generating in a seperate thread
	CHUNK_STATE_READY,      // generation finished
} TerrainChunkState;

typedef enum {
	STAGE_VOID,     // initial air, can be overridden by anything
	STAGE_TERRAIN,  // basic terrain, can be overridden by anything except the void
	STAGE_TREES,    // trees replace terrain
	STAGE_PLAYER,   // player-placed nodes or things placed after terrain generation
} TerrainGenStage;

typedef struct {
	TerrainGenStage tgs;
	List *changed_chunks;
} TerrainSetNodeArg;

typedef struct {
	pthread_mutex_t mtx;        // UwU please hit me senpai
	Blob data;                  // the big cum
	TerrainChunkState state;    // generation state of the chunk
	pthread_t gen_thread;       // thread that is generating chunk
	TerrainGenStageBuffer tgsb; // buffer to make sure terraingen only overrides things it should
} TerrainChunkMeta; // OMG META VERSE WEB 3.0 VIRTUAL REALITY

/*
	Locking conventions:
	- chunk lock protects chunk->data and meta->tgsb
	- meta mutex protects everything else in meta
	- if both meta mutex and chunk are going to be locked, meta must be locked first
	- you may not lock multiple meta mutexes at once
	- if multiple chunk locks are being obtained at once, EDEADLK must be handled
	- when locking a single chunk, assert return value of zero

	After changing the data in a chunk:
	1. release chunk lock
	2.
		- if meta mutex is currently locked: use server_terrain_send_chunk
		- if meta mutex is not locked: use server_terrain_lock_and_send_chunk

	If an operation affects multiple nodes (potentially in multiple chunks):
		- create a list changed_chunks
		- do job as normal, release individual chunk locks immediately after modifying their data
		- use server_terrain_lock_and_send_chunks to clear the list

	Note: Unless changed_chunks is given to server_terrain_gen_node, it sends chunks automatically
*/

// terrain object, data is stored here
extern Terrain *server_terrain;

// called on server startup
void server_terrain_init();
// called on server shutdown
void server_terrain_deinit();
// handle chunk request from client (thread safe)
void server_terrain_requested_chunk(ServerPlayer *player, v3s32 pos);
// prepare spawn region
void server_terrain_prepare_spawn();
// delete old node and put new
void server_terrain_replace_node(TerrainNode *ptr, TerrainNode new);
// set node with terraingen stage
void server_terrain_gen_node(v3s32 pos, TerrainNode node, TerrainGenStage new_tgs, List *changed_chunks);
// get the spawn height because idk
s32 server_terrain_spawn_height();
// when bit chunkus changes
void server_terrain_send_chunk(TerrainChunk *chunk);
// lock and send
void server_terrain_lock_and_send_chunk(TerrainChunk *chunk);
// lock and send multiple chunks at once
void server_terrain_lock_and_send_chunks(List *list);

#endif // _SERVER_TERRAIN_H_
