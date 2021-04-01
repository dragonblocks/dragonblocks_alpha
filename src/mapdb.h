#ifndef _MAPDB_H_
#define _MAPDB_H_

#include <sqlite3.h>
#include <stdbool.h>
#include "map.h"

sqlite3 *open_mapdb(const char *path);
bool load_block(sqlite3 *db, MapBlock *block);
void save_block(sqlite3 *db, MapBlock *block);

#endif
