#ifndef _DATABASE_H_
#define _DATABASE_H_

#include <stdbool.h>
#include "common/terrain.h"
#include "types.h"

void database_init();                                                  // open and initialize SQLite3 databases
void database_deinit();                                                // close databases
bool database_load_chunk(TerrainChunk *chunk);                         // load a chunk from terrain database (initializes state, tgs buffer and data), returns false on failure
void database_save_chunk(TerrainChunk *chunk);                         // save a chunk to terrain database
bool database_load_meta(const char *key, s64 *value_ptr);              // load a meta entry
void database_save_meta(const char *key, s64 value);                   // save / update a meta entry
bool database_load_player(char *name, v3f64 *pos, v3f32 *rot);         // load player data from database
void database_create_player(char *name, v3f64 pos, v3f32 rot);         // insert new player into database
void database_update_player_pos_rot(char *name, v3f64 pos, v3f32 rot); // update player position

#endif // _DATABASE_H_
