#ifndef _SERVER_TERRAIN_H_
#define _SERVER_TERRAIN_H_

#include <pthread.h>
#include "server/server_player.h"
#include "terrain.h"
#include "types.h"

typedef enum {
	CHUNK_CREATED,    // chunk exists but was not yet generated
	CHUNK_GENERATING, // currently generating in a seperate thread
	CHUNK_READY,      // generation finished
} TerrainChunkState;

typedef enum {
	STAGE_VOID,     // initial air, can be overridden by anything
	STAGE_TERRAIN,  // basic terrain, can be overridden by anything except the void
	STAGE_BOULDERS, // boulders, replace terrain
	STAGE_TREES,    // trees replace boulders
	STAGE_PLAYER,   // player-placed nodes or things placed after terrain generation
} TerrainGenStage;

typedef struct {
	TerrainGenStage tgs;
	List *changed_chunks;
} TerrainSetNodeArg;

typedef struct {
	Blob data;                  // the big cum
	TerrainChunkState state;    // generation state of the chunk
	pthread_t gen_thread;       // thread that is generating chunk
	TerrainGenStageBuffer tgsb; // buffer to make sure terraingen only overrides things it should
} TerrainChunkMeta; // OMG META VERSE WEB 3.0 VIRTUAL REALITY

extern Terrain *server_terrain; // terrain object, data is stored here

void server_terrain_init();                                                                           // called on server startup
void server_terrain_deinit();                                                                         // called on server shutdown
void server_terrain_requested_chunk(ServerPlayer *player, v3s32 pos);                                 // handle chunk request from client (thread safe)
void server_terrain_prepare_spawn();                                                                  // prepare spawn region
void server_terrain_gen_node(v3s32 pos, TerrainNode node, TerrainGenStage tgs, List *changed_chunks); // set node with terraingen stage
s32 server_terrain_spawn_height();                                                                    // get the spawn height because idk

#endif // _SERVER_TERRAIN_H_
