#ifndef _MAPDB_H_
#define _MAPDB_H_

#include <stdbool.h>
#include "map.h"

void database_init();										// open and initialize world SQLite3 database
void database_deinit();										// close database
bool database_load_block(MapBlock *block);					// load a block from map database (initializes state, mgs buffer and data), returns false on failure
void database_save_block(MapBlock *block);					// save a block to database
bool database_load_player(char *name, v3f64 *pos_ptr);		// load player data from database
void database_create_player(char *name, v3f64 pos);			// insert new player into database
void database_update_player_pos(char *name, v3f64 pos);		// update player position
#endif
