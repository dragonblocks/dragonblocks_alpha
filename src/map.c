#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "binsearch.h"
#include "map.h"
#include "util.h"

Map *map_create()
{
	Map *map = malloc(sizeof(Map));
	map->sectors = array_create(sizeof(MapSector *));
	return map;
}

static MapBlock **get_block_ptr(MapSector *sector, size_t idx)
{
	return (MapBlock **) sector->blocks.ptr + idx;
}

static MapSector **get_sector_ptr(Map *map, size_t idx)
{
	return (MapSector **) map->sectors.ptr + idx;
}

void map_delete(Map *map)
{
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = *get_sector_ptr(map, s);
		for (size_t b = 0; b < sector->blocks.siz; b++)
			map_free_block(*get_block_ptr(sector, b));
		if (sector->blocks.ptr)
			free(sector->blocks.ptr);
		free(sector);
	}
	if (map->sectors.ptr)
		free(map->sectors.ptr);
	free(map);
}

#define CMPBOUNDS(x) x == 0 ? 0 : x > 0 ? 1 : -1

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
		return *get_sector_ptr(map, res.index);
	if (! create)
		return NULL;

	MapSector *sector = malloc(sizeof(MapSector));
	sector->pos = pos;
	sector->hash = hash;
	sector->blocks = array_create(sizeof(MapBlock *));

	array_insert(&map->sectors, &sector, res.index);

	return sector;
}

static s8 block_compare(void *level, void *block)
{
	s32 d = *((s32 *) level) - ((MapBlock *) block)->pos.y;
	return CMPBOUNDS(d);
}

static MapBlock *allocate_block(v3s32 pos)
{
	MapBlock *block = malloc(sizeof(MapBlock));
	block->pos = pos;
	block->extra = NULL;
	return block;
}

MapBlock *map_get_block(Map *map, v3s32 pos, bool create)
{
	MapSector *sector = map_get_sector(map, (v2s32) {pos.x, pos.z}, create);
	if (! sector)
		return NULL;

	BinsearchResult res = binsearch(&pos.y, sector->blocks.ptr, sector->blocks.siz, &block_compare);

	MapBlock *block = NULL;

	if (res.success) {
		block = *get_block_ptr(sector, res.index);
	} else if (create) {
		block = allocate_block(pos);

		if (map->on_block_create)
			map->on_block_create(block);

		array_insert(&sector->blocks, &block, res.index);
	} else {
		return NULL;
	}

	return block->ready ? block : NULL;
}

void map_add_block(Map *map, MapBlock *block)
{
	MapSector *sector = map_get_sector(map, (v2s32) {block->pos.x, block->pos.z}, true);
	BinsearchResult res = binsearch(&block->pos.y, sector->blocks.ptr, sector->blocks.siz, &block_compare);
	if (res.success) {
		MapBlock **ptr = get_block_ptr(sector, res.index);
		map_free_block(*ptr);
		*ptr = block;
	} else {
		array_insert(&sector->blocks, &block, res.index);
	}
	if (map->on_block_add)
		map->on_block_add(block);
}

void map_clear_block(MapBlock *block, v3u8 init_state)
{
	for (u8 x = 0; x <= init_state.x; x++)
		for (u8 y = 0; y <= init_state.y; y++)
			for (u8 z = 0; z <= init_state.z; z++)
				map_node_clear(&block->data[x][y][z]);
}

void map_free_block(MapBlock *block)
{
	map_clear_block(block, (v3u8) {15, 15, 15});
	free(block);
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

MapBlock *map_deserialize_block(int fd)
{
	v3s32 pos;

	if (! read_v3s32(fd, &pos))
		return NULL;

	MapBlock *block = allocate_block(pos);

	ITERATE_MAPBLOCK {
		if (! map_deserialize_node(fd, &block->data[x][y][z])) {
			map_clear_block(block, (v3u8) {x, y, z});
			free(block);
			return NULL;
		}
	}

	return block;
}

bool map_serialize(int fd, Map *map)
{
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = *get_sector_ptr(map, s);
		for (size_t b = 0; b < sector->blocks.siz; b++)
			if (! map_serialize_block(fd, *get_block_ptr(sector, b)))
				return false;
	}
	return true;
}

void map_deserialize(int fd, Map *map)
{
	MapBlock *block;

	while ((block = map_deserialize_block(fd)) != NULL)
		map_add_block(map, block);
}

v3s32 map_node_to_block_pos(v3s32 pos, v3u8 *offset)
{
	if (offset)
		*offset = (v3u8) {(u32) pos.x % 16, (u32) pos.y % 16, (u32) pos.z % 16};
	return (v3s32) {floor((double) pos.x / 16.0), floor((double) pos.y / 16.0), floor((double) pos.z / 16.0)};
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
	MapBlock *block = map_get_block(map, map_node_to_block_pos(pos, &offset), false);
	if (block) {
		MapNode *current_node = &block->data[offset.x][offset.y][offset.z];
		map_node_clear(current_node);
		*current_node = node;
	}
}

MapNode map_node_create(Node type)
{
	return (MapNode) {type, list_create(&list_compare_string)};
}

void map_node_clear(MapNode *node)
{
	list_clear(&node->meta);
}
