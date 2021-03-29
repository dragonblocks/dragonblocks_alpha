#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <endian.h>
#include "map.h"
#include "util.h"

#define CMPBOUNDS(x) x == 0 ? 0 : x > 0 ? 1 : -1

static s8 sector_compare(void *hash, void *sector)
{
	s64 d = *((u64 *) hash) - (*(MapSector **) sector)->hash;
	return CMPBOUNDS(d);
}

static s8 block_compare(void *level, void *block)
{
	s32 d = *((s32 *) level) - (*(MapBlock **) block)->pos.y;
	return CMPBOUNDS(d);
}

static MapBlock *allocate_block(v3s32 pos)
{
	MapBlock *block = malloc(sizeof(MapBlock));
	block->pos = pos;
	block->state = MBS_CREATED;
	block->extra = NULL;
	pthread_mutex_init(&block->mtx, NULL);
	return block;
}

Map *map_create()
{
	Map *map = malloc(sizeof(Map));
	map->sectors = array_create(sizeof(MapSector *));
	map->sectors.cmp = &sector_compare;
	return map;
}

void map_delete(Map *map)
{
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = map_get_sector_raw(map, s);
		for (size_t b = 0; b < sector->blocks.siz; b++)
			map_free_block(map_get_block_raw(sector, b));
		if (sector->blocks.ptr)
			free(sector->blocks.ptr);
		free(sector);
	}
	if (map->sectors.ptr)
		free(map->sectors.ptr);
	free(map);
}

MapSector *map_get_sector_raw(Map *map, size_t idx)
{
	return ((MapSector **) map->sectors.ptr)[idx];
}

MapBlock *map_get_block_raw(MapSector *sector, size_t idx)
{
	return ((MapBlock **) sector->blocks.ptr)[idx];
}

MapSector *map_get_sector(Map *map, v2s32 pos, bool create)
{
	u64 hash = ((u64) pos.x << 32) + (u64) pos.y;
	ArraySearchResult res = array_search(&map->sectors, &hash);

	if (res.success)
		return map_get_sector_raw(map, res.index);
	if (! create)
		return NULL;

	MapSector *sector = malloc(sizeof(MapSector));
	sector->pos = pos;
	sector->hash = hash;
	sector->blocks = array_create(sizeof(MapBlock *));
	sector->blocks.cmp = &block_compare;

	array_insert(&map->sectors, &sector, res.index);

	return sector;
}

MapBlock *map_get_block(Map *map, v3s32 pos, bool create)
{
	MapSector *sector = map_get_sector(map, (v2s32) {pos.x, pos.z}, create);
	if (! sector)
		return NULL;

	ArraySearchResult res = array_search(&sector->blocks, &pos.y);

	MapBlock *block = NULL;

	if (res.success) {
		block = map_get_block_raw(sector, res.index);
	} else if (create) {
		block = allocate_block(pos);
		array_insert(&sector->blocks, &block, res.index);
	} else {
		return NULL;
	}

	return (create || block->state == MBS_READY) ? block : NULL;
}

void map_free_block(MapBlock *block)
{
	ITERATE_MAPBLOCK list_clear(&block->metadata[x][y][z]);
	pthread_mutex_destroy(&block->mtx);
	free(block);
}

bool map_deserialize_node(int fd, MapNode *node)
{
	Node type;

	if (! read_u32(fd, &type))
		return false;

	if (type >= NODE_UNLOADED)
		type = NODE_INVALID;

	*node = map_node_create(type);

	return true;
}

bool map_serialize_block(FILE *file, MapBlock *block)
{
	s32 pos[3] = {
		htobe32(block->pos.x),
		htobe32(block->pos.y),
		htobe32(block->pos.z),
	};
	if (fwrite(pos, 1, sizeof(pos), file) != sizeof(pos))
		return false;

	if (fwrite(block->data, 1, sizeof(block->data), file) !=  sizeof(block->data))
		perror("fwrite");
	else
		return true;

	return false;
}

bool map_deserialize_block(int fd, Map *map, MapBlock **blockptr, bool dummy)
{
	v3s32 pos;

	if (! read_v3s32(fd, &pos))
		return false;

	MapSector *sector = map_get_sector(map, (v2s32) {pos.x, pos.z}, true);
	ArraySearchResult res = array_search(&sector->blocks, &pos.y);

	MapBlock *block;

	if (dummy) {
		block = allocate_block(pos);
	} else if (res.success) {
		block = map_get_block_raw(sector, res.index);
	} else {
		block = allocate_block(pos);
		array_insert(&sector->blocks, &block, res.index);
	}

	bool ret = true;
	size_t n_read_total = 0;
	int n_read;

	while (n_read_total < sizeof(block->data)) {
		if ((n_read = read(fd, (char *) block->data + n_read_total, sizeof(block->data) - n_read_total)) == -1) {
			perror("read");
			ret = false;
			break;
		}

		n_read_total += n_read;
	}

	ITERATE_MAPBLOCK {
		if (block->data[x][y][z].type >= NODE_UNLOADED)
			block->data[x][y][z].type = NODE_INVALID;
		block->metadata[x][y][z] = list_create(&list_compare_string);
	}

	// ToDo: Deserialize meta

	if (dummy)
		map_free_block(block);
	else if (blockptr)
		*blockptr = block;

	return ret;
}

bool map_serialize(FILE *file, Map *map)
{
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = map_get_sector_raw(map, s);
		for (size_t b = 0; b < sector->blocks.siz; b++)
			if (! map_serialize_block(file, map_get_block_raw(sector, b)))
				return true;
	}
	return true;
}

void map_deserialize(int fd, Map *map)
{
	while (map_deserialize_block(fd, map, NULL, false))
		;
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
		list_clear(&block->metadata[offset.x][offset.y][offset.z]);
		block->data[offset.x][offset.y][offset.z] = node;

		block->state = MBS_MODIFIED;
	}
}

MapNode map_node_create(Node type)
{
	return (MapNode) {type};
}
