#ifndef _MAPDB_H_
#define _MAPDB_H_

#include <sqlite3.h>
#include <stdbool.h>
#include "map.h"

sqlite3 *mapdb_open(const char *path);
bool mapdb_load_block(sqlite3 *db, MapBlock *block);
void mapdb_save_block(sqlite3 *db, MapBlock *block);

#endif
