#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "terrain.h"

static void delete_chunk(TerrainChunk *chunk, Terrain *terrain)
{
	if (terrain->callbacks.delete_chunk)
		terrain->callbacks.delete_chunk(chunk);

	terrain_free_chunk(chunk);
}

static void delete_sector(TerrainSector *sector, Terrain *terrain)
{
	tree_clr(&sector->chunks, (void *) &delete_chunk, terrain, NULL, 0);
	pthread_rwlock_destroy(&sector->lock);
	free(sector);
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
	tree_clr(&terrain->sectors, (void *) &delete_sector, terrain, NULL, 0);
	pthread_rwlock_destroy(&terrain->lock);
	pthread_rwlock_destroy(&terrain->cache_lock);
	free(terrain);
}

TerrainSector *terrain_get_sector(Terrain *terrain, v2s32 pos, bool create)
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

TerrainChunk *terrain_get_chunk(Terrain *terrain, v3s32 pos, bool create)
{
	TerrainChunk *cache = NULL;

	pthread_rwlock_rdlock(&terrain->cache_lock);
	cache = terrain->cache;
	pthread_rwlock_unlock(&terrain->cache_lock);

	if (cache && v3s32_equals(cache->pos, pos))
		return cache;

	TerrainSector *sector = terrain_get_sector(terrain, (v2s32) {pos.x, pos.z}, create);
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

		pthread_mutex_lock(&chunk->mtx);
		if (terrain->callbacks.get_chunk && !terrain->callbacks.get_chunk(chunk, create)) {
			pthread_mutex_unlock(&chunk->mtx);
			chunk = NULL;
		} else {
			pthread_mutex_unlock(&chunk->mtx);
			pthread_rwlock_wrlock(&terrain->cache_lock);
			terrain->cache = chunk;
			pthread_rwlock_unlock(&terrain->cache_lock);
		}
	} else if (create) {
		tree_nmk(&sector->chunks, loc, chunk = terrain_allocate_chunk(pos));

		if (terrain->callbacks.create_chunk)
			terrain->callbacks.create_chunk(chunk);
	}

	pthread_rwlock_unlock(&sector->lock);

	return chunk;
}

TerrainChunk *terrain_allocate_chunk(v3s32 pos)
{
	TerrainChunk *chunk = malloc(sizeof * chunk);
	chunk->level = pos.y;
	chunk->pos = pos;
	chunk->extra = NULL;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&chunk->mtx, &attr);

	CHUNK_ITERATE
		chunk->data[x][y][z] = terrain_node_create(NODE_UNKNOWN, (Blob) {0, NULL});

	return chunk;
}

void terrain_free_chunk(TerrainChunk *chunk)
{
	CHUNK_ITERATE
		terrain_node_delete(chunk->data[x][y][z]);

	pthread_mutex_destroy(&chunk->mtx);
	free(chunk);
}

Blob terrain_serialize_chunk(TerrainChunk *chunk)
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

	SerializedTerrainChunk chunk_data;

	CHUNK_ITERATE {
		TerrainNode *node = &chunk->data[x][y][z];
		SerializedTerrainNode *node_data = &chunk_data.raw.nodes[x][y][z];

		*node_data = (SerializedTerrainNode) {
			.type = node->type,
			.data = {
				.siz = 0,
				.data = NULL,
			},
		};

		NodeDefinition *def = &node_definitions[node->type];

		if (def->serialize)
			def->serialize(&node_data->data, node->data);
	}

	Blob buffer = {0, NULL};
	SerializedTerrainChunk_write(&buffer, &chunk_data);
	SerializedTerrainChunk_free(&chunk_data);

	return buffer;
}

bool terrain_deserialize_chunk(TerrainChunk *chunk, Blob buffer)
{
	if (buffer.siz == 0) {
		CHUNK_ITERATE
			chunk->data[x][y][z] = terrain_node_create(NODE_AIR, (Blob) {0, NULL});

		return true;
	}

	// it's important to copy Blobs that have been malloc'd before reading from them
	// because reading from a Blob modifies its data and size pointer,
	// but does not free anything
	SerializedTerrainChunk chunk_data = {0};
	bool success = SerializedTerrainChunk_read(&buffer, &chunk_data);

	if (success) CHUNK_ITERATE
		chunk->data[x][y][z] = terrain_node_create(chunk_data.raw.nodes[x][y][z].type, chunk_data.raw.nodes[x][y][z].data);

	SerializedTerrainChunk_free(&chunk_data);
	return success;
}

v3s32 terrain_node_to_chunk_pos(v3s32 pos, v3u8 *offset)
{
	if (offset)
		*offset = (v3u8) {(u32) pos.x % CHUNK_SIZE, (u32) pos.y % CHUNK_SIZE, (u32) pos.z % CHUNK_SIZE};
	return (v3s32) {floor((double) pos.x / (double) CHUNK_SIZE), floor((double) pos.y / (double) CHUNK_SIZE), floor((double) pos.z / (double) CHUNK_SIZE)};
}

TerrainNode terrain_get_node(Terrain *terrain, v3s32 pos)
{
	v3u8 offset;
	v3s32 chunkpos = terrain_node_to_chunk_pos(pos, &offset);
	TerrainChunk *chunk = terrain_get_chunk(terrain, chunkpos, false);
	if (!chunk)
		return terrain_node_create(NODE_UNLOADED, (Blob) {0, NULL});
	return chunk->data[offset.x][offset.y][offset.z];
}

void terrain_set_node(Terrain *terrain, v3s32 pos, TerrainNode node, bool create, void *arg)
{
	v3u8 offset;
	TerrainChunk *chunk = terrain_get_chunk(terrain, terrain_node_to_chunk_pos(pos, &offset), create);

	if (!chunk)
		return;

	pthread_mutex_lock(&chunk->mtx);
	if (!terrain->callbacks.set_node || terrain->callbacks.set_node(chunk, offset, &node, arg)) {
		chunk->data[offset.x][offset.y][offset.z] = node;
		if (terrain->callbacks.after_set_node)
			terrain->callbacks.after_set_node(chunk, offset, arg);
	} else {
		terrain_node_delete(node);
	}
	pthread_mutex_unlock(&chunk->mtx);
}

TerrainNode terrain_node_create(NodeType type, Blob buffer)
{
	if (type >= NODE_UNLOADED)
		type = NODE_UNKNOWN;

	NodeDefinition *def = &node_definitions[type];

	TerrainNode node;
	node.type = type;
	node.data = def->data_size ? malloc(def->data_size) : NULL;

	if (def->create)
		def->create(&node);

	if (def->deserialize)
		def->deserialize(&buffer, node.data);

	return node;
}

void terrain_node_delete(TerrainNode node)
{
	NodeDefinition *def = &node_definitions[node.type];

	if (def->delete)
		def->delete(&node);

	if (node.data)
		free(node.data);
}
