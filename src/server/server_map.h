#ifndef _SERVER_MAP_H_
#define _SERVER_MAP_H_

#include <stddef.h>
#include <pthread.h>
#include "map.h"
#include "server/server.h"
#include "server/mapdb.h"

typedef enum
{
	MBS_CREATED,	// block exists but was not yet generated
	MBS_GENERATING,	// currently generating in a seperate thread
	MBS_READY,		// generation finished
} MapBlockState;

typedef enum
{
	MGS_VOID,		// initial air, can be overridden by anything
	MGS_TERRAIN,	// basic terrain, can be overridden by anything except the void
	MGS_BOULDERS,	// boulders, replace terrain
	MGS_PLAYER,		// player-placed nodes or things placed after map generation
} MapgenStage;

typedef MapgenStage MapgenStageBuffer[MAPBLOCK_SIZE][MAPBLOCK_SIZE][MAPBLOCK_SIZE];

typedef struct {
	MapgenStage mgs;
	List *changed_blocks;
} MapgenSetNodeArg;

typedef struct
{
	char *data;						// cached serialized data
	size_t size;					// size of data
	MapBlockState state;			// generation state of the block
	pthread_t mapgen_thread;		// thread that is generating block
	MapgenStageBuffer mgs_buffer;	// buffer to make sure mapgen only overrides things it should
} MapBlockExtraData;

extern struct ServerMap {
	Map *map;								// map object, data is stored here
	sqlite3 *db;							// SQLite3 database to save data to database
	bool shutting_down;						// is a shutdown in progress?
	List mapgen_threads;					// a list of mapgen threads (need to be joined before shutdown)
	pthread_mutex_t mapgen_threads_mtx;		// mutex to protect mapgen thread list
} server_map; // ServerMap singleton

void server_map_init(Server *server);						// ServerMap singleton constructor
void server_map_deinit();									// ServerMap singleton destructor
void server_map_requested_block(Client *client, v3s32 pos);	// handle block request from client (thread safe)

#endif
