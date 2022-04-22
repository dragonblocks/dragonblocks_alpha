#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "terrain.h"

typedef struct {
	v2s32 pos;
	Tree chunks;
	pthread_rwlock_t lock;
} TerrainSector;

static TerrainChunk *allocate_chunk(v3s32 pos)
{
	TerrainChunk *chunk = malloc(sizeof * chunk);
	chunk->level = pos.y;
	chunk->pos = pos;
	chunk->extra = NULL;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	pthread_mutex_init(&chunk->mtx, &attr);

	CHUNK_ITERATE
		chunk->data[x][y][z] = (TerrainNode) {NODE_UNKNOWN, NULL};

	return chunk;
}

static void free_chunk(Terrain *terrain, TerrainChunk *chunk)
{
	if (terrain->callbacks.delete_node) CHUNK_ITERATE
		terrain->callbacks.delete_node(&chunk->data[x][y][z]);

	pthread_mutex_destroy(&chunk->mtx);
	free(chunk);
}

static void delete_chunk(TerrainChunk *chunk, Terrain *terrain)
{
	if (terrain->callbacks.delete_chunk)
		terrain->callbacks.delete_chunk(chunk);

	free_chunk(terrain, chunk);
}

static void delete_sector(TerrainSector *sector, Terrain *terrain)
{
	tree_clr(&sector->chunks, &delete_chunk, terrain, NULL, 0);
	pthread_rwlock_destroy(&sector->lock);
	free(sector);
}

static TerrainSector *get_sector(Terrain *terrain, v2s32 pos, bool create)
{
	if (create)
		pthread_rwlock_wrlock(&terrain->lock);
	else
		pthread_rwlock_rdlock(&terrain->lock);

	TreeNode **loc = tree_nfd(&terrain->sectors, &pos, &v2s32_cmp);
	TerrainSector *sector = NULL;

	if (*loc) {
		sector = (*loc)->dat;
	} else if (create) {
		sector = malloc(sizeof *sector);
		sector->pos = pos;
		tree_ini(&sector->chunks);
		pthread_rwlock_init(&sector->lock, NULL);

		tree_nmk(&terrain->sectors, loc, sector);
	}

	pthread_rwlock_unlock(&terrain->lock);

	return sector;
}

Terrain *terrain_create()
{
	Terrain *terrain = malloc(sizeof *terrain);
	tree_ini(&terrain->sectors);
	pthread_rwlock_init(&terrain->lock, NULL);
	terrain->cache = NULL;
	pthread_rwlock_init(&terrain->cache_lock, NULL);
	return terrain;
}

void terrain_delete(Terrain *terrain)
{
	tree_clr(&terrain->sectors, &delete_sector, terrain, NULL, 0);
	pthread_rwlock_destroy(&terrain->lock);
	pthread_rwlock_destroy(&terrain->cache_lock);
	free(terrain);
}

TerrainChunk *terrain_get_chunk(Terrain *terrain, v3s32 pos, bool create)
{
	TerrainChunk *cache = NULL;

	pthread_rwlock_rdlock(&terrain->cache_lock);
	cache = terrain->cache;
	pthread_rwlock_unlock(&terrain->cache_lock);

	if (cache && v3s32_equals(cache->pos, pos))
		return cache;

	TerrainSector *sector = get_sector(terrain, (v2s32) {pos.x, pos.z}, create);
	if (!sector)
		return NULL;

	if (create)
		pthread_rwlock_wrlock(&sector->lock);
	else
		pthread_rwlock_rdlock(&sector->lock);

	TreeNode **loc = tree_nfd(&sector->chunks, &pos.y, &s32_cmp);
	TerrainChunk *chunk = NULL;

	if (*loc) {
		chunk = (*loc)->dat;

		if (terrain->callbacks.get_chunk && !terrain->callbacks.get_chunk(chunk, create)) {
			chunk = NULL;
		} else {
			pthread_rwlock_wrlock(&terrain->cache_lock);
			terrain->cache = chunk;
			pthread_rwlock_unlock(&terrain->cache_lock);
		}
	} else if (create) {
		tree_nmk(&sector->chunks, loc, chunk = allocate_chunk(pos));

		if (terrain->callbacks.create_chunk)
			terrain->callbacks.create_chunk(chunk);
	}

	pthread_rwlock_unlock(&sector->lock);

	return chunk;
}

TerrainChunk *terrain_get_chunk_nodep(Terrain *terrain, v3s32 nodep, v3s32 *offset, bool create)
{
	TerrainChunk *chunk = terrain_get_chunk(terrain, terrain_chunkp(nodep), create);
	if (!chunk)
		return NULL;
	*offset = terrain_offset(nodep);
	return chunk;
}

Blob terrain_serialize_chunk(__attribute__((unused)) Terrain *terrain, TerrainChunk *chunk, void (*callback)(TerrainNode *node, Blob *buffer))
{
	bool empty = true;

	CHUNK_ITERATE {
		if (chunk->data[x][y][z].type != NODE_AIR) {
			empty = false;
			break;
		}
	}

	if (empty)
		return (Blob) {0, NULL};

	SerializedTerrainChunk serialized_chunk;

	CHUNK_ITERATE {
		TerrainNode *node = &chunk->data[x][y][z];
		SerializedTerrainNode *serialized = &serialized_chunk.raw.nodes[x][y][z];

		serialized->type = node->type;
		serialized->data = (Blob) {0, NULL};

		if (callback)
			callback(node, &serialized->data);
	}

	Blob buffer = {0, NULL};
	SerializedTerrainChunk_write(&buffer, &serialized_chunk);
	SerializedTerrainChunk_free(&serialized_chunk);
	return buffer;
}

bool terrain_deserialize_chunk(Terrain *terrain, TerrainChunk *chunk, Blob buffer, void (*callback)(TerrainNode *node, Blob buffer))
{
	if (buffer.siz == 0) {
		CHUNK_ITERATE {
			if (terrain->callbacks.delete_node)
				terrain->callbacks.delete_node(&chunk->data[x][y][z]);

			chunk->data[x][y][z] = (TerrainNode) {NODE_AIR, NULL};
		}
		return true;
	}

	// it's important to copy Blobs that have been malloc'd before reading from them
	// because reading from a Blob modifies its data and size pointer,
	// but does not free anything
	SerializedTerrainChunk serialized_chunk = {0};
	bool success = SerializedTerrainChunk_read(&buffer, &serialized_chunk);

	if (success) CHUNK_ITERATE {
		if (terrain->callbacks.delete_node)
			terrain->callbacks.delete_node(&chunk->data[x][y][z]);

		TerrainNode *node = &chunk->data[x][y][z];
		SerializedTerrainNode *serialized = &serialized_chunk.raw.nodes[x][y][z];

		node->type = serialized->type;

		if (callback)
			callback(node, serialized->data);
	}

	SerializedTerrainChunk_free(&serialized_chunk);
	return success;
}

void terrain_lock_chunk(TerrainChunk *chunk)
{
	if (pthread_mutex_lock(&chunk->mtx) == 0)
		return;

	fprintf(stderr, "[error] failed to lock terrain chunk mutex\n");
	abort();
}

TerrainNode terrain_get_node(Terrain *terrain, v3s32 pos)
{
	v3s32 offset;
	TerrainChunk *chunk = terrain_get_chunk_nodep(terrain, pos, &offset, false);
	if (!chunk)
		return (TerrainNode) {COUNT_NODE, NULL};

	terrain_lock_chunk(chunk);
	TerrainNode node = chunk->data[offset.x][offset.y][offset.z];
	pthread_mutex_unlock(&chunk->mtx);

	return node;
}

v3s32 terrain_chunkp(v3s32 pos)
{
	return (v3s32) {
		floor((double) pos.x / (double) CHUNK_SIZE),
		floor((double) pos.y / (double) CHUNK_SIZE),
		floor((double) pos.z / (double) CHUNK_SIZE)};
}

v3s32 terrain_offset(v3s32 pos)
{
	return (v3s32) {
		(u32) pos.x % CHUNK_SIZE,
		(u32) pos.y % CHUNK_SIZE,
		(u32) pos.z % CHUNK_SIZE};
}
