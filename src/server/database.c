#include <stdio.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "server/database.h"
#include "server/server_map.h"
#include "util.h"

static sqlite3 *database;

// utility functions

// print SQLite3 error message for failed block SQL statement
static void print_block_error(MapBlock *block, const char *action)
{
	printf("Database error with %s block at (%d, %d, %d): %s\n", action, block->pos.x, block->pos.y, block->pos.z, sqlite3_errmsg(database));
}

// prepare a SQLite3 block statement and bind the position
static sqlite3_stmt *prepare_block_statement(MapBlock *block, const char *action, const char *sql)
{
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		print_block_error(block, action);
		return NULL;
	}

	size_t psize = sizeof(s32) * 3;
	s32 *pos = malloc(psize);
	pos[0] = htobe32(block->pos.x);
	pos[1] = htobe32(block->pos.y);
	pos[2] = htobe32(block->pos.z);

	sqlite3_bind_blob(stmt, 1, pos, psize, &free);

	return stmt;
}

// public functions

// open and initialize world SQLite3 database
void database_init()
{
	char *err;

	if (sqlite3_open_v2("world.sqlite", &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
		printf("Failed to open database: %s\n", sqlite3_errmsg(database));
	} else if (sqlite3_exec(database, "CREATE TABLE IF NOT EXISTS map (pos BLOB PRIMARY KEY, generated INT, size INT, data BLOB, mgsb_size INT, mgsb_data BLOB);", NULL, NULL, &err) != SQLITE_OK) {
		printf("Failed to initialize database: %s\n", err);
		sqlite3_free(err);
	}
}

// close database
void database_deinit()
{
	sqlite3_close(database);
}

// load a block from map database (initializes state, mgs buffer and data), returns false on failure
bool database_load_block(MapBlock *block)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_block_statement(block, "loading", "SELECT generated, size, data, mgsb_size, mgsb_data FROM map WHERE pos=?")))
		return false;

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found) {
		MapBlockExtraData *extra = block->extra;

		extra->state = sqlite3_column_int(stmt, 0) ? MBS_READY : MBS_CREATED;
		extra->size = sqlite3_column_int64(stmt, 1);
		extra->data = malloc(extra->size);
		memcpy(extra->data, sqlite3_column_blob(stmt, 2), extra->size);

		MapgenStageBuffer decompressed_mgsb;
		my_decompress(sqlite3_column_blob(stmt, 4), sqlite3_column_int64(stmt, 3), &decompressed_mgsb, sizeof(MapgenStageBuffer));

		ITERATE_MAPBLOCK extra->mgs_buffer[x][y][z] = be32toh(decompressed_mgsb[x][y][z]);

		if (! map_deserialize_block(block, extra->data, extra->size))
			printf("Error with deserializing block at (%d, %d, %d)\n", block->pos.x, block->pos.y, block->pos.z);
	} else if (rc != SQLITE_DONE) {
		print_block_error(block, "loading");
	}

	sqlite3_finalize(stmt);
	return found;
}

// save a block to database
void database_save_block(MapBlock *block)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_block_statement(block, "saving", "REPLACE INTO map (pos, generated, size, data, mgsb_size, mgsb_data) VALUES(?1, ?2, ?3, ?4, ?5, ?6)")))
		return;

	MapBlockExtraData *extra = block->extra;

	MapgenStageBuffer uncompressed_mgsb;
	ITERATE_MAPBLOCK uncompressed_mgsb[x][y][z] = htobe32(extra->mgs_buffer[x][y][z]);

	char *mgsb_data;
	size_t mgsb_size;

	my_compress(&uncompressed_mgsb, sizeof(MapgenStageBuffer), &mgsb_data, &mgsb_size);

	sqlite3_bind_int(stmt, 2, extra->state > MBS_CREATED);
	sqlite3_bind_int64(stmt, 3, extra->size);
	sqlite3_bind_blob(stmt, 4, extra->data, extra->size, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 5, mgsb_size);
	sqlite3_bind_blob(stmt, 6, mgsb_data, mgsb_size, &free);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		print_block_error(block, "saving");

	sqlite3_finalize(stmt);
}
