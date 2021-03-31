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

static MapBlock *allocate_block(v3s32 pos)
{
	MapBlock *block = malloc(sizeof(MapBlock));
	block->pos = pos;
	block->state = MBS_CREATED;
	block->extra = NULL;
	pthread_mutex_init(&block->mtx, NULL);
	return block;
}

static MapBlock *create_block(MapSector *sector, size_t idx, v3s32 pos)
{
	MapBlock *block = allocate_block(pos);
	array_insert(&sector->blocks, &block, idx);
	return block;
}

static bool read_block_data(int fd, MapBlockData data)
{
	size_t n_read_total = 0;
	int n_read;

	while (n_read_total < sizeof(MapBlockData)) {
		if ((n_read = read(fd, (char *) data + n_read_total, sizeof(MapBlockData) - n_read_total)) == -1) {
			perror("read");
			return false;
		}

		n_read_total += n_read;
	}

	return true;
}

Map *map_create()
{
	Map *map = malloc(sizeof(Map));
	pthread_rwlock_init(&map->rwlck, NULL);
	map->sectors = array_create(sizeof(MapSector *));
	map->sectors.cmp = &sector_compare;
	return map;
}

void map_delete(Map *map)
{
	pthread_rwlock_destroy(&map->rwlck);
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = map_get_sector_raw(map, s);
		for (size_t b = 0; b < sector->blocks.siz; b++)
			map_free_block(map_get_block_raw(sector, b));
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
		block = create_block(sector, res.index, pos);
	}

	pthread_rwlock_unlock(&sector->rwlck);

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

bool map_serialize_block(FILE *file, MapBlock *block)
{
	s32 pos[3] = {
		htobe32(block->pos.x),
		htobe32(block->pos.y),
		htobe32(block->pos.z),
	};
	if (fwrite(pos, 1, sizeof(pos), file) != sizeof(pos))
		return false;

	MapBlockData data;

	ITERATE_MAPBLOCK {
		MapNode node = block->data[x][y][z];
		node.type = htobe32(node.type);
		data[x][y][z] = node;
	}

	if (fwrite(data, 1, sizeof(MapBlockData), file) !=  sizeof(MapBlockData))
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

	MapBlock *block;

	if (dummy)
		block = allocate_block(pos);
	else
		block = map_get_block(map, pos, true);

	if (block->state != MBS_CREATED)
		map_clear_meta(block);

	pthread_mutex_lock(&block->mtx);
	block->state = MBS_INITIALIZING;

	MapBlockData data;

	bool success = read_block_data(fd, data);

	ITERATE_MAPBLOCK {
		if (success) {
			MapNode node = data[x][y][z];
			node.type = be32toh(node.type);

			if (node.type >= NODE_UNLOADED)
				node.type = NODE_INVALID;

			block->data[x][y][z] = node;
		}

		block->metadata[x][y][z] = list_create(&list_compare_string);
	}

	block->state = MBS_UNSENT;

	if (dummy)
		map_free_block(block);
	else if (blockptr && success)
		*blockptr = block;

	pthread_mutex_unlock(&block->mtx);

	return success;
}

bool map_serialize(FILE *file, Map *map)
{
	pthread_rwlock_rdlock(&map->rwlck);
	for (size_t s = 0; s < map->sectors.siz; s++) {
		MapSector *sector = map_get_sector_raw(map, s);
		pthread_rwlock_rdlock(&sector->rwlck);
		for (size_t b = 0; b < sector->blocks.siz; b++)
			if (! map_serialize_block(file, map_get_block_raw(sector, b)))
				return true;
		pthread_rwlock_unlock(&sector->rwlck);
	}
	pthread_rwlock_unlock(&map->rwlck);
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

		block->state = MBS_UNSENT;
	}
}

MapNode map_node_create(Node type)
{
	return (MapNode) {type};
}
