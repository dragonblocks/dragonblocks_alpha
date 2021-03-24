#ifndef _MAP_H_
#define _MAP_H_

#include <stdbool.h>
#include "array.h"
#include "linkedlist.h"
#include "node.h"
#include "types.h"

#define ITERATE_MAPBLOCK for (u8 x = 0; x < 16; x++) for (u8 y = 0; y < 16; y++) for (u8 z = 0; z < 16; z++)

typedef struct
{
	Node type;
	LinkedList meta;
} MapNode;

typedef struct
{
	MapNode data[16][16][16];
	v3s32 pos;
	void *extra;
} MapBlock;

typedef struct
{
	Array blocks;
	v2s32 pos;
	u64 hash;
} MapSector;

typedef struct
{
	Array sectors;
} Map;

MapSector *map_get_sector(Map *map, v2s32 pos, bool create);
MapBlock *map_get_block(Map *map, v3s32 pos, bool create);

void map_clear_block(MapBlock *block, v3u8 init_state);
void map_free_block(MapBlock *block);
void map_add_block(Map *map, MapBlock *block);

bool map_deserialize_node(int fd, MapNode *buf);
bool map_serialize_block(int fd, MapBlock *block);
MapBlock *map_deserialize_block(int fd);
bool map_serialize(int fd, Map *map);
void map_deserialize(int fd, Map *map);

v3s32 map_node_to_block_pos(v3s32 pos, v3u8 *offset);

MapNode map_get_node(Map *map, v3s32 pos);
void map_set_node(Map *map, v3s32 pos, MapNode node);
MapNode map_node_create(Node type);
void map_node_clear(MapNode *node);

Map *map_create();
void map_delete(Map *map);

#endif
