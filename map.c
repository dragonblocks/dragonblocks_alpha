#include <stdlib.h>
#include <stdbool.h>
#include "map.h"
#include "binsearch.h"

#define CMPBOUNDS(x) x == 0 ? 0 : x > 0 ? 1 : -1

static void map_raw_delete_block(MapBlock *block)
{
	ITERATE_MAPBLOCK map_node_clear(&block->data[x][y][z]);
	free(block);
}

static s8 sector_compare(void *hash, void *sector)
{
	s64 d = *((u64 *) hash) - ((MapSector *) sector)->hash;
	return CMPBOUNDS(d);
}

MapSector *map_get_sector(Map *map, v2s32 pos, bool create)
{
	u64 hash = ((u64) pos.x << 32) + (u64) pos.y;
	BinsearchResult res = binsearch(&hash, map->sectors.ptr, map->sectors.siz, &sector_compare);

	if (res.success)
		return map->sectors.ptr[res.index];
	if (! create)
		return NULL;

	MapSector *sector = malloc(sizeof(MapSector));
	sector->pos = pos;
	sector->hash = hash;
	sector->blocks = array_create();

	array_insert(&map->sectors, sector, res.index);

	return sector;
}

static s8 block_compare(void *level, void *block)
{
	s32 d = *((s32 *) level) - ((MapSector *) block)->pos.y;
	return CMPBOUNDS(d);
}

MapBlock *map_get_block(Map *map, v3s32 pos, bool create)
{
	MapSector *sector = map_get_sector(map, (v2s32) {pos.x, pos.z}, create);
	if (! sector)
		return NULL;

	BinsearchResult res = binsearch(&pos.y, sector->blocks.ptr, sector->blocks.siz, &block_compare);

	if (res.success)
		return sector->blocks.ptr[res.index];
	if (! create)
		return NULL;

	MapBlock *block = malloc(sizeof(MapBlock));
	block->pos = pos;

	MapNode air = map_node_create(NODE_AIR);
	ITERATE_MAPBLOCK block->data[x][y][z] = air;

	array_insert(&sector->blocks, block, res.index);

	return block;
}

MapNode map_get_node(Map *map, v3s32 pos)
{
	MapBlock *block = map_get_block(map, (v3s32) {pos.x / 16, pos.y / 16, pos.z / 16}, false);
	if (! block)
		return map_node_create(NODE_UNLOADED);
	return block->data[pos.x % 16][pos.y % 16][pos.z % 16];
}

void map_set_node(Map *map, v3s32 pos, MapNode node)
{
	MapBlock *block = map_get_block(map, (v3s32) {pos.x / 16, pos.y / 16, pos.z / 16}, true);
	MapNode *current_node = &block->data[pos.x % 16][pos.y % 16][pos.z % 16];
	map_node_clear(current_node);
	*current_node = node;
}

MapNode map_node_create(Node type)
{
	return (MapNode) {type, linked_list_create()};
}

void map_node_clear(MapNode *node)
{
	linked_list_clear(&node->meta);
}

Map *map_create(FILE *file)
{
	Map *map = malloc(sizeof(Map));
	map->file = file;
	map->sectors = array_create();

	return map;
}

void map_delete(Map *map)
{
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = map->sectors.ptr[s];
		for (size_t b = 0; b < sector->blocks.siz; b++)
			map_raw_delete_block(sector->blocks.ptr[b]);
		if (sector->blocks.ptr)
			free(sector->blocks.ptr);
		free(sector);
	}
	if (map->sectors.ptr)
		free(map->sectors.ptr);
	free(map);
}
