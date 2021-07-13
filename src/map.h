#ifndef _MAP_H_
#define _MAP_H_

#include <stdbool.h>
#include <pthread.h>
#include "bintree.h"
#include "list.h"
#include "node.h"
#include "types.h"

#define ITERATE_MAPBLOCK for (u8 x = 0; x < 16; x++) for (u8 y = 0; y < 16; y++) for (u8 z = 0; z < 16; z++)

typedef struct MapNode
{
	Node type;
	NodeState state;
} MapNode;

typedef enum
{
	MBS_CREATED,
	MBS_READY,
	MBS_MODIFIED,
	MBS_PROCESSING,
} MapBlockState;

typedef MapNode MapBlockData[16][16][16];

typedef u32 MapBlockHeader;

typedef struct
{
	MapBlockData data;
	List metadata[16][16][16];
	v3s32 pos;
	MapBlockState state;
	pthread_mutex_t mtx;
	void *extra;
	void (*free_extra)(void *ptr);
} MapBlock;

typedef struct
{
	pthread_rwlock_t rwlck;
	Bintree blocks;
	v2s32 pos;
	u64 hash;
} MapSector;

typedef struct
{
	pthread_rwlock_t rwlck;
	Bintree sectors;
	pthread_rwlock_t cached_rwlck;
	MapBlock *cached;
} Map;

Map *map_create();
void map_delete(Map *map);

MapSector *map_get_sector(Map *map, v2s32 pos, bool create);
MapBlock *map_get_block(Map *map, v3s32 pos, bool create);

MapBlock *map_allocate_block(v3s32 pos);
void map_clear_meta(MapBlock *block);
void map_free_block(MapBlock *block);

bool map_deserialize_node(int fd, MapNode *buf);
void map_serialize_block(MapBlock *block, char **dataptr, size_t *sizeptr);
bool map_deserialize_block(MapBlock *block, const char *data, size_t size);

v3s32 map_node_to_block_pos(v3s32 pos, v3u8 *offset);

MapNode map_get_node(Map *map, v3s32 pos);
void map_set_node(Map *map, v3s32 pos, MapNode node);
MapNode map_node_create(Node type);
void map_node_clear(MapNode *node);

#endif
