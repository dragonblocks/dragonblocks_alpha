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

typedef struct {
	s32 level;
	v3s32 pos;
	TerrainNode data[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	void *extra;
	pthread_mutex_t mtx;
} TerrainChunk;

typedef struct {
	Tree sectors;
	pthread_rwlock_t lock;
	TerrainChunk *cache;
	pthread_rwlock_t cache_lock;
	struct {
		void (*create_chunk)(TerrainChunk *chunk);
		void (*delete_chunk)(TerrainChunk *chunk);
		bool (*get_chunk)(TerrainChunk *chunk, bool create);
		void (*delete_node)(TerrainNode *node);
	} callbacks;
} Terrain;

Terrain *terrain_create();
void terrain_delete(Terrain *terrain);

TerrainChunk *terrain_get_chunk(Terrain *terrain, v3s32 pos, bool create);
TerrainChunk *terrain_get_chunk_nodep(Terrain *terrain, v3s32 node_pos, v3s32 *offset, bool create);

Blob terrain_serialize_chunk(Terrain *terrain, TerrainChunk *chunk, void (*callback)(TerrainNode *node, Blob *buffer));
bool terrain_deserialize_chunk(Terrain *terrain, TerrainChunk *chunk, Blob buffer, void (*callback)(TerrainNode *node, Blob buffer));

TerrainNode terrain_get_node(Terrain *terrain, v3s32 pos);

void terrain_lock_chunk(TerrainChunk *chunk);

v3s32 terrain_chunkp(v3s32 pos);
v3s32 terrain_offset(v3s32 pos);

#endif
