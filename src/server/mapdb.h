#ifndef _MAPDB_H_
#define _MAPDB_H_

#include <sqlite3.h>
#include <stdbool.h>
#include "map.h"

sqlite3 *mapdb_open(const char *path);					// open and initialize SQLite3 database at path
bool mapdb_load_block(sqlite3 *db, MapBlock *block);	// load a block from map database (initializes state, mgs buffer and data), returns false on failure
void mapdb_save_block(sqlite3 *db, MapBlock *block);	// save a block to database

#endif
