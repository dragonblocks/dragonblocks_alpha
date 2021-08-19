#include <stdio.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "server/mapdb.h"
#include "server/server_map.h"
#include "util.h"

// utility functions

// print SQLite3 error message for failed block SQL statement
static void print_error(sqlite3 *db, MapBlock *block, const char *action)
{
	printf("Database error with %s block at (%d, %d, %d): %s\n", action, block->pos.x, block->pos.y, block->pos.z, sqlite3_errmsg(db));
}

// prepare a SQLite3 block statement and bind the position
static sqlite3_stmt *prepare_statement(sqlite3 *db, MapBlock *block, const char *action, const char *sql)
{
	sqlite3_stmt *stmt;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
		print_error(db, block, action);
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

// open and initialize SQLite3 database at path
sqlite3 *mapdb_open(const char *path)
{
	sqlite3 *db;
	char *err;

	if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
		printf("Failed to open database: %s\n", sqlite3_errmsg(db));
	} else if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS blocks (pos BLOB PRIMARY KEY, generated INT, size INT, data BLOB, mgsb_size INT, mgsb_data BLOB);", NULL, NULL, &err) != SQLITE_OK) {
		printf("Failed to initialize database: %s\n", err);
		sqlite3_free(err);
	}

	return db;
}

// load a block from map database (initializes state, mgs buffer and data), returns false on failure
bool mapdb_load_block(sqlite3 *db, MapBlock *block)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement(db, block, "loading", "SELECT generated, size, data, mgsb_size, mgsb_data FROM blocks WHERE pos=?")))
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
		print_error(db, block, "loading");
	}

	sqlite3_finalize(stmt);
	return found;
}

// save a block to database
void mapdb_save_block(sqlite3 *db, MapBlock *block)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement(db, block, "saving", "REPLACE INTO blocks (pos, generated, size, data, mgsb_size, mgsb_data) VALUES(?1, ?2, ?3, ?4, ?5, ?6)")))
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
		print_error(db, block, "saving");

	sqlite3_finalize(stmt);
}
