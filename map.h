#ifndef _MAP_H_
#define _MAP_H_

#include <stdio.h>
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
	FILE *file;
} Map;

MapSector *map_get_sector(Map *map, v2s32 pos, bool create);
MapBlock *map_get_block(Map *map, v3s32 pos, bool create);
void map_create_block(Map *map, v3s32 pos, MapBlock *block);

void map_serialize_block(int fd, MapBlock *); // ToDo
MapBlock *map_deserialize_block(int fd); // ToDo

void map_delete_block(MapBlock *); // ToDo
void map_unload_block(MapBlock *); // ToDo

MapNode map_get_node(Map *map, v3s32 pos);
void map_set_node(Map *map, v3s32 pos, MapNode node);
MapNode map_node_create(Node type);
void map_node_clear(MapNode *node);

Map *map_create(FILE *file);
void map_delete(Map *map);

#endif
