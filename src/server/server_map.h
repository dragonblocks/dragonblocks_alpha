#ifndef _SERVER_MAP_H_
#define _SERVER_MAP_H_

#include <stddef.h>
#include <pthread.h>
#include "map.h"
#include "server/server_player.h"
#include "types.h"

typedef enum
{
	MBS_CREATED,    // block exists but was not yet generated
	MBS_GENERATING, // currently generating in a seperate thread
	MBS_READY,      // generation finished
} MapBlockState;

typedef enum
{
	MGS_VOID,     // initial air, can be overridden by anything
	MGS_TERRAIN,  // basic terrain, can be overridden by anything except the void
	MGS_BOULDERS, // boulders, replace terrain
	MGS_TREES,    // trees replace boulders
	MGS_PLAYER,   // player-placed nodes or things placed after map generation
} MapgenStage;

typedef struct {
	MapgenStage mgs;
	List *changed_blocks;
} MapgenSetNodeArg;

typedef struct
{
	Blob data;                    // the big cum
	MapBlockState state;          // generation state of the block
	pthread_t mapgen_thread;      // thread that is generating block
	MapgenStageBuffer mgsb;       // buffer to make sure mapgen only overrides things it should
} MapBlockExtraData;

extern struct ServerMap {
	Map *map;                            // map object, data is stored here
	bool joining_threads;                // prevent threads from removing themselves from the thread list if thread list is being cleared anyway
	pthread_mutex_t joining_threads_mtx; // mutex to protect joining threads
	List mapgen_threads;                 // a list of mapgen threads (need to be joined before shutdown)
	pthread_mutex_t mapgen_threads_mtx;  // mutex to protect mapgen thread list
	s32 spawn_height;                    // height to spawn players at
} server_map; // ServerMap singleton

void server_map_init();                                           // ServerMap singleton constructor
void server_map_deinit();                                         // ServerMap singleton destructor
void server_map_requested_block(ServerPlayer *player, v3s32 pos); // handle block request from client (thread safe)
void server_map_prepare_spawn();                                  // prepare spawn region

#endif
