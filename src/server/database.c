#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include "common/day.h"
#include "common/perlin.h"
#include "server/database.h"
#include "server/server_node.h"
#include "server/server_terrain.h"

static sqlite3 *terrain_database;
static sqlite3 *meta_database;
static sqlite3 *players_database;

// utility functions

// prepare a SQLite3 statement
static inline sqlite3_stmt *prepare_statement(sqlite3 *database, const char *sql)
{
	sqlite3_stmt *stmt;
	return sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) == SQLITE_OK ? stmt : NULL;
}

// print SQLite3 error message for failed chunk SQL statement
static inline void print_chunk_error(TerrainChunk *chunk, const char *action)
{
	fprintf(stderr, "[warning] failed %s chunk at (%d, %d, %d): %s\n", action, chunk->pos.x, chunk->pos.y, chunk->pos.z, sqlite3_errmsg(terrain_database));
}

// prepare a SQLite3 chunk statement and bind the position
static sqlite3_stmt *prepare_chunk_statement(TerrainChunk *chunk, const char *action, const char *sql)
{
	sqlite3_stmt *stmt = prepare_statement(terrain_database, sql);

	if (!stmt) {
		print_chunk_error(chunk, action);
		return NULL;
	}

	Blob buffer = {0, NULL};
	v3s32_write(&buffer, &chunk->pos);

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

// bind v3f32 to sqlite3 statement
static inline void bind_v3f32(sqlite3_stmt *stmt, int idx, v3f32 pos)
{
	Blob buffer = {0, NULL};
	v3f32_write(&buffer, &pos);

	sqlite3_bind_blob(stmt, idx, buffer.data, buffer.siz, &free);
}

// public functions

// open and initialize SQLite3 databases
void database_init(const char *world_path)
{
	struct {
		sqlite3 **handle;
		const char *path;
		const char *init;
	} databases[3] = {
		{&terrain_database, "terrain.sqlite", "CREATE TABLE IF NOT EXISTS terrain (pos  BLOB PRIMARY KEY, generated INTEGER, data BLOB, tgsb BLOB);"},
		{&meta_database,    "meta.sqlite",    "CREATE TABLE IF NOT EXISTS meta    (key  TEXT PRIMARY KEY, value INTEGER                          );"},
		{&players_database, "players.sqlite", "CREATE TABLE IF NOT EXISTS players (name TEXT PRIMARY KEY, pos BLOB, rot BLOB                     );"},
	};

	for (int i = 0; i < 3; i++) {
		char path[strlen(world_path) + 1 + strlen(databases[i].path) + 1];
		sprintf(path, "%s/%s", world_path, databases[i].path);

		if (sqlite3_open_v2(path, databases[i].handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
			fprintf(stderr, "[error] failed to open %s: %s\n", path, sqlite3_errmsg(*databases[i].handle));
			abort();
		}

		char *err;
		if (sqlite3_exec(*databases[i].handle, databases[i].init, NULL, NULL, &err) != SQLITE_OK) {
			fprintf(stderr, "[error] failed initializing %s: %s\n", path, err);
			sqlite3_free(err);
			abort();
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

// close databases
void database_deinit()
{
	database_save_meta("time_of_day", (s64) get_time_of_day());

	sqlite3_close(terrain_database);
	sqlite3_close(meta_database);
	sqlite3_close(players_database);
}

// load a chunk from terrain database (initializes state, tgs buffer and data), returns false on failure
bool database_load_chunk(TerrainChunk *chunk)
{
	sqlite3_stmt *stmt = prepare_chunk_statement(chunk, "loading", "SELECT generated, data, tgsb FROM terrain WHERE pos=?");

	if (!stmt)
		return false;

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found) {
		TerrainChunkMeta *meta = chunk->extra;

		meta->state = sqlite3_column_int(stmt, 0) ? CHUNK_STATE_READY : CHUNK_STATE_CREATED;
		Blob data = {sqlite3_column_bytes(stmt, 1), (void *) sqlite3_column_blob(stmt, 1)};
		Blob tgsb = {sqlite3_column_bytes(stmt, 2), (void *) sqlite3_column_blob(stmt, 2)};

		TerrainGenStageBuffer_read(&tgsb, &meta->tgsb);
		if (!terrain_deserialize_chunk(server_terrain, chunk, data, &server_node_deserialize)) {
			fprintf(stderr, "[error] failed deserializing chunk at (%d, %d, %d)\n", chunk->pos.x, chunk->pos.y, chunk->pos.z);
			abort();
		}
	} else if (rc != SQLITE_DONE) {
		print_chunk_error(chunk, "loading");
	}

	sqlite3_finalize(stmt);
	return found;
}

// save a chunk to terrain database
void database_save_chunk(TerrainChunk *chunk)
{
	sqlite3_stmt *stmt = prepare_chunk_statement(chunk, "saving", "REPLACE INTO terrain (pos, generated, data, tgsb) VALUES(?1, ?2, ?3, ?4)");

	if (!stmt)
		return;

	TerrainChunkMeta *meta = chunk->extra;

	Blob data = terrain_serialize_chunk(server_terrain, chunk, &server_node_serialize);

	Blob tgsb = {0, NULL};
	TerrainGenStageBuffer_write(&tgsb, &meta->tgsb);

	sqlite3_bind_int(stmt, 2, meta->state > CHUNK_STATE_CREATED);
	sqlite3_bind_blob(stmt, 3, data.data, data.siz, &free);
	sqlite3_bind_blob(stmt, 4, tgsb.data, tgsb.siz, &free);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		print_chunk_error(chunk, "saving");

	sqlite3_finalize(stmt);
}

// load a meta entry
bool database_load_meta(const char *key, s64 *value_ptr)
{
	sqlite3_stmt *stmt = prepare_statement(meta_database, "SELECT value FROM meta WHERE key=?");

	if (!stmt) {
		fprintf(stderr, "[warning] failed loading meta %s: %s\n", key, sqlite3_errmsg(meta_database));
		return false;
	}

	sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_TRANSIENT);

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found)
		*value_ptr = sqlite3_column_int64(stmt, 0);
	else if (rc != SQLITE_DONE)
		fprintf(stderr, "[warning] failed loading meta %s: %s\n", key, sqlite3_errmsg(meta_database));

	sqlite3_finalize(stmt);
	return found;
}

// save / update a meta entry
void database_save_meta(const char *key, s64 value)
{
	sqlite3_stmt *stmt = prepare_statement(meta_database, "REPLACE INTO meta (key, value) VALUES(?1, ?2)");

	if (!stmt) {
		fprintf(stderr, "[warning] failed saving meta %s: %s\n", key, sqlite3_errmsg(meta_database));
		return;
	}

	sqlite3_bind_text(stmt, 1, key, strlen(key), SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, value);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		fprintf(stderr, "[warning] failed saving meta %s: %s\n", key, sqlite3_errmsg(meta_database));

	sqlite3_finalize(stmt);
}

// load player data from database
bool database_load_player(char *name, v3f64 *pos, v3f32 *rot)
{
	sqlite3_stmt *stmt = prepare_statement(players_database, "SELECT pos, rot FROM players WHERE name=?");

	if (!stmt) {
		fprintf(stderr, "[warning] failed loading player %s: %s\n", name, sqlite3_errmsg(players_database));
		return false;
	}

	sqlite3_bind_text(stmt, 1, name, strlen(name), SQLITE_TRANSIENT);

	int rc = sqlite3_step(stmt);
	bool found = rc == SQLITE_ROW;

	if (found) {
		v3f64_read(&(Blob) {sqlite3_column_bytes(stmt, 0), (void *) sqlite3_column_blob(stmt, 0)}, pos);
		v3f32_read(&(Blob) {sqlite3_column_bytes(stmt, 1), (void *) sqlite3_column_blob(stmt, 1)}, rot);
	} else if (rc != SQLITE_DONE) {
		fprintf(stderr, "[warning] failed loading player %s: %s\n", name, sqlite3_errmsg(players_database));
	}

	sqlite3_finalize(stmt);
	return found;
}

// insert new player into database
void database_create_player(char *name, v3f64 pos, v3f32 rot)
{
	sqlite3_stmt *stmt = prepare_statement(players_database, "INSERT INTO players (name, pos, rot) VALUES(?1, ?2, ?3)");

	if (!stmt) {
		fprintf(stderr, "[warning] failed creating player %s: %s\n", name, sqlite3_errmsg(players_database));
		return;
	}

	sqlite3_bind_text(stmt, 1, name, strlen(name), SQLITE_TRANSIENT);
	bind_v3f64(stmt, 2, pos);
	bind_v3f32(stmt, 3, rot);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		fprintf(stderr, "[warning] failed creating player %s: %s\n", name, sqlite3_errmsg(players_database));

	sqlite3_finalize(stmt);
}

// update player position
void database_update_player_pos_rot(char *name, v3f64 pos, v3f32 rot)
{
	sqlite3_stmt *stmt = prepare_statement(players_database, "UPDATE players SET pos=?1, rot=?2 WHERE name=?3");

	if (!stmt) {
		fprintf(stderr, "[warning] failed updating player %s: %s\n", name, sqlite3_errmsg(players_database));
		return;
	}

	bind_v3f64(stmt, 1, pos);
	bind_v3f32(stmt, 2, rot);
	sqlite3_bind_text(stmt, 3, name, strlen(name), SQLITE_TRANSIENT);

	if (sqlite3_step(stmt) != SQLITE_DONE)
		fprintf(stderr, "[warning] failed updating player %s: %s\n", name, sqlite3_errmsg(players_database));

	sqlite3_finalize(stmt);
}
