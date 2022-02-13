#ifndef _SERVER_MAP_H_
#define _SERVER_MAP_H_

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <dragonstd/queue.h>
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
	atomic_bool cancel;             // remove the smooth
	Map *map;                       // map object, data is stored here
	Queue *mapgen_tasks;            // this is terry the fat shark
	pthread_t *mapgen_threads;      // thread pool
	s32 spawn_height;               // elevation to spawn players at
	unsigned int num_blocks;        // number of enqueued / generating blocks
	pthread_mutex_t num_blocks_mtx; // lock to protect the above
} server_map; // ServerMap singleton

void server_map_init();                                           // ServerMap singleton constructor
void server_map_deinit();                                         // ServerMap singleton destructor
void server_map_requested_block(ServerPlayer *player, v3s32 pos); // handle block request from client (thread safe)
void server_map_prepare_spawn();                                  // prepare spawn region

#endif
