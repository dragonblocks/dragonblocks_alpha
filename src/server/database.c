#include <stdio.h>
#include <endian.h/endian.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include "day.h"
#include "server/database.h"
#include "server/server_map.h"
#include "perlin.h"
#include "util.h"

static sqlite3 *database;

// utility functions

// prepare a SQLite3 statement
static inline sqlite3_stmt *prepare_statement(const char *sql)
{
	sqlite3_stmt *stmt;
	return sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) == SQLITE_OK ? stmt : NULL;
}

// print SQLite3 error message for failed block SQL statement
static inline void print_block_error(MapBlock *block, const char *action)
{
	fprintf(stderr, "Database error with %s block at (%d, %d, %d): %s\n", action, block->pos.x, block->pos.y, block->pos.z, sqlite3_errmsg(database));
}

// prepare a SQLite3 block statement and bind the position
static sqlite3_stmt *prepare_block_statement(MapBlock *block, const char *action, const char *sql)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement(sql))) {
		print_block_error(block, action);
		return NULL;
	}

	Blob buffer = {0, NULL};
	v3s32_write(&buffer, &block->pos);

	sqlite3_bind_blob(stmt, 1, buffer.data, buffer.siz, &free);

	return stmt;
}

// bind v3f64 to sqlite3 statement
static inline void bind_v3f64(sqlite3_stmt *stmt, int idx, v3f64 pos)
{
	Blob buffer = {0, NULL};
	v3f64_write(&buffer, &pos);

	sqlite3_bind_blob(stmt, idx, buffer.data, buffer.siz, &free);
}

// public functions

// open and initialize world SQLite3 database
void database_init()
{
	char *err;

	if (sqlite3_open_v2("world.sqlite", &database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
		fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(database));
		return;
	}

	const char *init_stmts[3]= {
		"CREATE TABLE IF NOT EXISTS map (pos BLOB PRIMARY KEY, generated INTEGER, data BLOB, mgsb BLOB);",
		"CREATE TABLE IF NOT EXISTS meta (key TEXT PRIMARY KEY, value INTEGER);",
		"CREATE TABLE IF NOT EXISTS players (name TEXT PRIMARY KEY, pos BLOB);"
	};

	for (int i = 0; i < 3; i++) {
		if (sqlite3_exec(database, init_stmts[i], NULL, NULL, &err) != SQLITE_OK) {
			fprintf(stderr, "Failed to initialize database: %s\n", err);
			sqlite3_free(err);
			return;
		}
	}

	s64 saved_seed;

	if (database_load_meta("seed", &saved_seed)) {
		seed = saved_seed;
	} else {
		srand(time(NULL));
		seed = rand();
		database_save_meta("seed", seed);
	}

	s64 time_of_day;

	if (database_load_meta("time_of_day", &time_of_day))
		set_time_of_day(time_of_day);
	else
		set_time_of_day(12 * MINUTES_PER_HOUR);
}

// close database
void database_deinit()
{
	database_save_meta("time_of_day", (s64) get_time_of_day());
	sqlite3_close(database);
}

// load a block from map database (initializes state, mgs buffer and data), returns false on failure
bool database_load_block(MapBlock *block)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_block_statement(block, "loading", "SELECT generated, data, mgsb FROM map WHERE pos=?")))
		return false;

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found) {
		MapBlockExtraData *extra = block->extra;

		extra->state = sqlite3_column_int(stmt, 0) ? MBS_READY : MBS_CREATED;
		Blob_read(             &(Blob) {sqlite3_column_bytes(stmt, 1), (void *) sqlite3_column_blob(stmt, 1)}, &extra->data);
		MapgenStageBuffer_read(&(Blob) {sqlite3_column_bytes(stmt, 2), (void *) sqlite3_column_blob(stmt, 2)}, &extra->mgsb);

		if (! map_deserialize_block(block, extra->data)) {
			fprintf(stderr, "Failed to load block at (%d, %d, %d)\n", block->pos.x, block->pos.y, block->pos.z);
			exit(EXIT_FAILURE);
		}
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

	if (! (stmt = prepare_block_statement(block, "saving", "REPLACE INTO map (pos, generated, data, mgsb) VALUES(?1, ?2, ?3, ?4)")))
		return;

	MapBlockExtraData *extra = block->extra;

	Blob data = {0, NULL};
	Blob_write(&data, &extra->data);

	Blob mgsb = {0, NULL};
	MapgenStageBuffer_write(&mgsb, &extra->mgsb);

	sqlite3_bind_int(stmt, 2, extra->state > MBS_CREATED);
	sqlite3_bind_blob(stmt, 3, data.data, data.siz, &free);
	sqlite3_bind_blob(stmt, 4, mgsb.data, mgsb.siz, &free);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		print_block_error(block, "saving");

	sqlite3_finalize(stmt);
}

// load a meta entry
bool database_load_meta(const char *key, s64 *value_ptr)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement("SELECT value FROM meta WHERE key=?"))) {
		fprintf(stderr, "Database error with loading %s: %s\n", key, sqlite3_errmsg(database));
		return false;
	}

	sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_TRANSIENT);

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found)
		*value_ptr = sqlite3_column_int64(stmt, 0);
	else if (rc != SQLITE_DONE)
		fprintf(stderr, "Database error with loading %s: %s\n", key, sqlite3_errmsg(database));

	sqlite3_finalize(stmt);
	return found;
}

// save / update a meta entry
void database_save_meta(const char *key, s64 value)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement("REPLACE INTO meta (key, value) VALUES(?1, ?2)"))) {
		fprintf(stderr, "Database error with saving %s: %s\n", key, sqlite3_errmsg(database));
		return;
	}

	sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, value);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		fprintf(stderr, "Database error with saving %s: %s\n", key, sqlite3_errmsg(database));

	sqlite3_finalize(stmt);
}

// load player data from database
bool database_load_player(char *name, v3f64 *pos_ptr)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement("SELECT pos FROM players WHERE name=?"))) {
		fprintf(stderr, "Database error with loading player %s: %s\n", name, sqlite3_errmsg(database));
		return false;
	}

	sqlite3_bind_text(stmt, 1, name, strlen(name), SQLITE_TRANSIENT);

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found)
		v3f64_read(&(Blob) {sqlite3_column_bytes(stmt, 0), (void *) sqlite3_column_blob(stmt, 0)}, pos_ptr);
	else if (rc != SQLITE_DONE)
		fprintf(stderr, "Database error with loading player %s: %s\n", name, sqlite3_errmsg(database));

	sqlite3_finalize(stmt);
	return found;
}

// insert new player into database
void database_create_player(char *name, v3f64 pos)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement("INSERT INTO players (name, pos) VALUES(?1, ?2)"))) {
		fprintf(stderr, "Database error with creating player %s: %s\n", name, sqlite3_errmsg(database));
		return;
	}

	sqlite3_bind_text(stmt, 1, name, strlen(name), SQLITE_TRANSIENT);
	bind_v3f64(stmt, 2, pos);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		fprintf(stderr, "Database error with creating player %s: %s\n", name, sqlite3_errmsg(database));

	sqlite3_finalize(stmt);
}

// update player position
void database_update_player_pos(char *name, v3f64 pos)
{
	sqlite3_stmt *stmt;

	if (! (stmt = prepare_statement("UPDATE players SET pos=?1 WHERE name=?2"))) {
		fprintf(stderr, "Database error with updating player %s position: %s\n", name, sqlite3_errmsg(database));
		return;
	}

	bind_v3f64(stmt, 1, pos);
	sqlite3_bind_text(stmt, 2, name, strlen(name), SQLITE_TRANSIENT);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		fprintf(stderr, "Database error with updating player %s position: %s\n", name, sqlite3_errmsg(database));

	sqlite3_finalize(stmt);
}
