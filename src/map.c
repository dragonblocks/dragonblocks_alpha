#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
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

Map *map_create()
{
	Map *map = malloc(sizeof(Map));
	pthread_rwlock_init(&map->rwlck, NULL);
	map->sectors = array_create(sizeof(MapSector *));
	map->sectors.cmp = &sector_compare;
	return map;
}

void map_delete(Map *map, void (*callback)(void *extra))
{
	pthread_rwlock_destroy(&map->rwlck);
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = map_get_sector_raw(map, s);
		for (size_t b = 0; b < sector->blocks.siz; b++) {
			MapBlock *block = map_get_block_raw(sector, b);
			if (callback)
				callback(block->extra);
			map_free_block(block);
		}
		if (sector->blocks.ptr)
			free(sector->blocks.ptr);
		pthread_rwlock_destroy(&sector->rwlck);
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

	if (create)
		pthread_rwlock_wrlock(&map->rwlck);
	else
		pthread_rwlock_rdlock(&map->rwlck);

	ArraySearchResult res = array_search(&map->sectors, &hash);

	MapSector *sector = NULL;

	if (res.success) {
		sector = map_get_sector_raw(map, res.index);
	} else if (create) {
		sector = malloc(sizeof(MapSector));
		pthread_rwlock_init(&sector->rwlck, NULL);
		sector->pos = pos;
		sector->hash = hash;
		sector->blocks = array_create(sizeof(MapBlock *));
		sector->blocks.cmp = &block_compare;

		array_insert(&map->sectors, &sector, res.index);
	}

	pthread_rwlock_unlock(&map->rwlck);

	return sector;
}

MapBlock *map_get_block(Map *map, v3s32 pos, bool create)
{
	MapSector *sector = map_get_sector(map, (v2s32) {pos.x, pos.z}, create);
	if (! sector)
		return NULL;

	if (create)
		pthread_rwlock_wrlock(&sector->rwlck);
	else
		pthread_rwlock_rdlock(&sector->rwlck);

	ArraySearchResult res = array_search(&sector->blocks, &pos.y);

	MapBlock *block = NULL;

	if (res.success) {
		block = map_get_block_raw(sector, res.index);
		if (block->state < MBS_READY && ! create)
			block = NULL;
	} else if (create) {
		block = map_allocate_block(pos);
		array_insert(&sector->blocks, &block, res.index);
	}

	pthread_rwlock_unlock(&sector->rwlck);

	return block;
}

MapBlock *map_allocate_block(v3s32 pos)
{
	MapBlock *block = malloc(sizeof(MapBlock));
	block->pos = pos;
	block->state = MBS_CREATED;
	block->extra = NULL;
	pthread_mutex_init(&block->mtx, NULL);
	return block;
}

void map_clear_meta(MapBlock *block)
{
	pthread_mutex_lock(&block->mtx);
	ITERATE_MAPBLOCK list_clear(&block->metadata[x][y][z]);
	pthread_mutex_unlock(&block->mtx);
}

void map_free_block(MapBlock *block)
{
	map_clear_meta(block);
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

void map_serialize_block(MapBlock *block, char **dataptr, size_t *sizeptr)
{
	*sizeptr = sizeof(MapBlockHeader) + sizeof(MapBlockData);
	*dataptr = malloc(*sizeptr);

	MapBlockHeader header = htobe64(sizeof(MapBlockData));
	memcpy(*dataptr, &header, sizeof(header));

	MapBlockData blockdata;
	ITERATE_MAPBLOCK {
		MapNode node = block->data[x][y][z];
		node.type = htobe32(node.type);
		blockdata[x][y][z] = node;
	}
	memcpy(*dataptr + sizeof(header), blockdata, sizeof(MapBlockData));
}

bool map_deserialize_block(MapBlock *block, const char *data, size_t size)
{
	if (size < sizeof(MapBlockData))
		return false;

	MapBlockData blockdata;

	memcpy(blockdata, data, sizeof(MapBlockData));

	ITERATE_MAPBLOCK {
		MapNode node = blockdata[x][y][z];
		node.type = be32toh(node.type);

		if (node.type >= NODE_UNLOADED)
			node.type = NODE_INVALID;

		block->data[x][y][z] = node;

		block->metadata[x][y][z] = list_create(&list_compare_string);
	}

	block->state = MBS_MODIFIED;

	return true;
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
		pthread_mutex_lock(&block->mtx);
		block->state = MBS_MODIFIED;
		list_clear(&block->metadata[offset.x][offset.y][offset.z]);
		block->data[offset.x][offset.y][offset.z] = node;
		pthread_mutex_unlock(&block->mtx);
	}
}

MapNode map_node_create(Node type)
{
	return (MapNode) {type};
}
