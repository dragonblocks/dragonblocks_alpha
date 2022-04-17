#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include <dragonstd/list.h>
#include <dragonstd/tree.h>
#include <stdbool.h>
#include <pthread.h>
#include "node.h"
#include "types.h"

#define CHUNK_ITERATE \
	for (s32 x = 0; x < CHUNK_SIZE; x++) \
	for (s32 y = 0; y < CHUNK_SIZE; y++) \
	for (s32 z = 0; z < CHUNK_SIZE; z++)

typedef struct TerrainNode {
	NodeType type;
	void *data;
} TerrainNode;

typedef TerrainNode TerrainChunkData[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];

typedef struct {
	s32 level;
	v3s32 pos;
	TerrainChunkData data;
	void *extra;
	pthread_mutex_t mtx;
} TerrainChunk;

typedef struct
{
	v2s32 pos;
	Tree chunks;
	pthread_rwlock_t lock;
} TerrainSector;

typedef struct {
	Tree sectors;
	pthread_rwlock_t lock;
	TerrainChunk *cache;
	pthread_rwlock_t cache_lock;
	struct {
		void (*create_chunk)(TerrainChunk *chunk);
		void (*delete_chunk)(TerrainChunk *chunk);
		bool (*get_chunk)(TerrainChunk *chunk, bool create);
		bool (*set_node) (TerrainChunk *chunk, v3u8 offset, TerrainNode *node, void *arg);
		void (*after_set_node)(TerrainChunk *chunk, v3u8 offset, void *arg);
	} callbacks;
} Terrain;

Terrain *terrain_create();
void terrain_delete(Terrain *terrain);

TerrainSector *terrain_get_sector(Terrain *terrain, v2s32 pos, bool create);
TerrainChunk *terrain_get_chunk(Terrain *terrain, v3s32 pos, bool create);

TerrainChunk *terrain_allocate_chunk(v3s32 pos);
void terrain_free_chunk(TerrainChunk *chunk);

Blob terrain_serialize_chunk(TerrainChunk *chunk);
bool terrain_deserialize_chunk(TerrainChunk *chunk, Blob buffer);

v3s32 terrain_node_to_chunk_pos(v3s32 pos, v3u8 *offset);

TerrainNode terrain_get_node(Terrain *terrain, v3s32 pos);
void terrain_set_node(Terrain *terrain, v3s32 pos, TerrainNode node, bool create, void *arg);

TerrainNode terrain_node_create(NodeType type, Blob buffer);
void terrain_node_delete(TerrainNode node);

#endif
