#ifndef _MAP_H_
#define _MAP_H_

#include <stdbool.h>
#include <pthread.h>
#include <dragonstd/bintree.h>
#include <dragonstd/list.h>
#include "types.h"
#include "node.h"

#define ITERATE_MAPBLOCK for (u8 x = 0; x < MAPBLOCK_SIZE; x++) for (u8 y = 0; y < MAPBLOCK_SIZE; y++) for (u8 z = 0; z < MAPBLOCK_SIZE; z++)

typedef struct MapNode
{
	Node type;
	void *data;
} MapNode;

typedef MapNode MapBlockData[MAPBLOCK_SIZE][MAPBLOCK_SIZE][MAPBLOCK_SIZE];

typedef struct
{
	MapBlockData data;
	v3s32 pos;
	pthread_mutex_t mtx;
	void *extra;
} MapBlock;

typedef struct
{
	pthread_rwlock_t rwlck;
	Bintree blocks;
	v2s32 pos;
} MapSector;

typedef struct
{
	void (*create_block)(MapBlock *block);
	void (*delete_block)(MapBlock *block);
	bool (*get_block)(MapBlock *block, bool create);
	bool (*set_node) (MapBlock *block, v3u8 offset, MapNode *node, void *arg);
	void (*after_set_node)(MapBlock *block, v3u8 offset, void *arg);
} MapCallbacks;

typedef struct
{
	pthread_rwlock_t rwlck;
	Bintree sectors;
	pthread_rwlock_t cached_rwlck;
	MapBlock *cached;
	MapCallbacks callbacks;
} Map;

Map *map_create(MapCallbacks callbacks);
void map_delete(Map *map);

MapSector *map_get_sector(Map *map, v2s32 pos, bool create);
MapBlock *map_get_block(Map *map, v3s32 pos, bool create);

MapBlock *map_allocate_block(v3s32 pos);
void map_free_block(MapBlock *block);

Blob map_serialize_block(MapBlock *block);
bool map_deserialize_block(MapBlock *block, Blob buffer);

v3s32 map_node_to_block_pos(v3s32 pos, v3u8 *offset);

MapNode map_get_node(Map *map, v3s32 pos);
void map_set_node(Map *map, v3s32 pos, MapNode node, bool create, void *arg);

MapNode map_node_create(Node type, Blob buffer);
void map_node_delete(MapNode node);

#endif
