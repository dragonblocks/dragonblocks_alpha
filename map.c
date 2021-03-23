#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "binsearch.h"
#include "map.h"
#include "util.h"

#define CMPBOUNDS(x) x == 0 ? 0 : x > 0 ? 1 : -1

static void raw_delete_block(MapBlock *block)
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
	s32 d = *((s32 *) level) - ((MapBlock *) block)->pos.y;
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

bool map_deserialize_node(int fd, MapNode *node)
{
	Node type;

	if (! read_u32(fd, &type))
		return false;

	if (type > NODE_INVALID)
		type = NODE_INVALID;

	*node = map_node_create(type);

	return true;
}

bool map_serialize_block(int fd, MapBlock *block)
{
	if (! write_v3s32(fd, block->pos))
		return false;

	ITERATE_MAPBLOCK {
		if (! write_u32(fd, block->data[x][y][z].type))
			return false;
	}

	return true;
}

bool map_deserialize_block(int fd, Map *map)
{
	MapBlock *block = malloc(sizeof(MapBlock));

	if (! read_v3s32(fd, &block->pos))
		return false;

	ITERATE_MAPBLOCK {
		if (! map_deserialize_node(fd, &block->data[x][y][z])) {
			free(block);
			return false;
		}
	}

	MapSector *sector = map_get_sector(map, (v2s32) {block->pos.x, block->pos.z}, true);
	BinsearchResult res = binsearch(&block->pos.y, sector->blocks.ptr, sector->blocks.siz, &block_compare);
	if (res.success) {
		raw_delete_block(sector->blocks.ptr[res.index]);
		sector->blocks.ptr[res.index] = block;
	} else {
		array_insert(&sector->blocks, block, res.index);
	}

	return true;
}


v3s32 map_node_to_block_pos(v3s32 pos, v3u8 *offset)
{
	if (offset)
		*offset = (v3u8) {(u32) pos.x % 16, (u32) pos.y % 16, (u32) pos.z % 16};
	return (v3s32) {floor((double) pos.x / 16), floor((double) pos.y / 16), floor((double) pos.z / 16)};
}


MapNode map_get_node(Map *map, v3s32 pos)
{
	v3u8 offset;
	v3s32 blockpos = map_node_to_block_pos(pos, &offset);
	MapBlock *block = map_get_block(map, blockpos, false);
	if (! block)
		return map_node_create(NODE_UNLOADED);
	return block->data[offset.x][offset.y][offset.z];
}

void map_set_node(Map *map, v3s32 pos, MapNode node)
{
	v3u8 offset;
	MapBlock *block = map_get_block(map, map_node_to_block_pos(pos, &offset), true);
	MapNode *current_node = &block->data[offset.x][offset.y][offset.z];
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

Map *map_create()
{
	Map *map = malloc(sizeof(Map));
	map->sectors = array_create();

	return map;
}

void map_delete(Map *map)
{
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = map->sectors.ptr[s];
		for (size_t b = 0; b < sector->blocks.siz; b++)
			raw_delete_block(sector->blocks.ptr[b]);
		if (sector->blocks.ptr)
			free(sector->blocks.ptr);
		free(sector);
	}
	if (map->sectors.ptr)
		free(map->sectors.ptr);
	free(map);
}
