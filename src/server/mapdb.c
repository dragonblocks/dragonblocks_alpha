#include <stdio.h>
#include <endian.h>
#include <stdlib.h>
#include "server/mapdb.h"
#include "server/server_map.h"

static void print_error(sqlite3 *db, MapBlock *block, const char *action)
{
	printf("Database error with %s block at %d %d %d: %s\n", action, block->pos.x, block->pos.y, block->pos.z, sqlite3_errmsg(db));
}

sqlite3 *mapdb_open(const char *path)
{
	sqlite3 *db;
	char *err;

	if (sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
		printf("Failed to open database: %s\n", sqlite3_errmsg(db));
	} else if (sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS blocks (pos BLOB PRIMARY KEY, data BLOB NOT NULL);", NULL, NULL, &err) != SQLITE_OK) {
		printf("Failed to initialize database: %s\n", err);
		sqlite3_free(err);
	}

	return db;
}

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

bool mapdb_load_block(sqlite3 *db, MapBlock *block)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement(db, block, "loading", "SELECT data FROM blocks WHERE pos=?")))
		return false;

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found) {
		const char *data = sqlite3_column_blob(stmt, 0);
		map_deserialize_block(block, data + sizeof(MapBlockHeader), be32toh(*(MapBlockHeader *) data));
	} else if (rc != SQLITE_DONE) {
		print_error(db, block, "loading");
	}

	sqlite3_finalize(stmt);
	return found;
}

void mapdb_save_block(sqlite3 *db, MapBlock *block)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement(db, block, "saving", "REPLACE INTO blocks (pos, data) VALUES(?1, ?2)")))
		return;

	MapBlockExtraData *extra = block->extra;

	sqlite3_bind_blob(stmt, 2, extra->data, extra->size, SQLITE_TRANSIENT);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		print_error(db, block, "saving");

	sqlite3_finalize(stmt);
}
