#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <endian.h>
#include <zlib.h>
#include "map.h"
#include "util.h"

Map *map_create()
{
	Map *map = malloc(sizeof(Map));
	pthread_rwlock_init(&map->rwlck, NULL);
	pthread_rwlock_init(&map->cached_rwlck, NULL);
	map->sectors = bintree_create(sizeof(v2s32));
	map->cached = NULL;
	return map;
}

static void free_block(void *block)
{
	map_free_block(block);
}

static void free_sector(void *sector_void)
{
	MapSector *sector = sector_void;
	bintree_clear(&sector->blocks, &free_block);
	pthread_rwlock_destroy(&sector->rwlck);
	free(sector);
}

void map_delete(Map *map)
{
	pthread_rwlock_destroy(&map->rwlck);
	pthread_rwlock_destroy(&map->cached_rwlck);
	bintree_clear(&map->sectors, &free_sector);
	free(map);
}

MapSector *map_get_sector(Map *map, v2s32 pos, bool create)
{
	if (create)
		pthread_rwlock_wrlock(&map->rwlck);
	else
		pthread_rwlock_rdlock(&map->rwlck);

	BintreeNode **nodeptr = bintree_search(&map->sectors, &pos);

	MapSector *sector = NULL;

	if (*nodeptr) {
		sector = (*nodeptr)->value;
	} else if (create) {
		sector = malloc(sizeof(MapSector));
		pthread_rwlock_init(&sector->rwlck, NULL);
		sector->pos = pos;
		sector->blocks = bintree_create(sizeof(s32));

		bintree_add_node(&map->sectors, nodeptr, &pos, sector);
	}

	pthread_rwlock_unlock(&map->rwlck);

	return sector;
}

MapBlock *map_get_block(Map *map, v3s32 pos, bool create)
{
	MapBlock *cached = NULL;

	pthread_rwlock_rdlock(&map->cached_rwlck);
	cached = map->cached;
	pthread_rwlock_unlock(&map->cached_rwlck);

	if (cached && cached->pos.x == pos.x && cached->pos.y == pos.y && cached->pos.z == pos.z)
		return cached;

	MapSector *sector = map_get_sector(map, (v2s32) {pos.x, pos.z}, create);
	if (! sector)
		return NULL;

	if (create)
		pthread_rwlock_wrlock(&sector->rwlck);
	else
		pthread_rwlock_rdlock(&sector->rwlck);

	BintreeNode **nodeptr = bintree_search(&sector->blocks, &pos.y);

	MapBlock *block = NULL;

	if (*nodeptr) {
		block = (*nodeptr)->value;

		if (block->state < MBS_READY) {
			if (! create)
				block = NULL;
		} else {
			pthread_rwlock_wrlock(&map->cached_rwlck);
			map->cached = block;
			pthread_rwlock_unlock(&map->cached_rwlck);
		}
	} else if (create) {
		block = map_allocate_block(pos);

		bintree_add_node(&sector->blocks, nodeptr, &pos.y, block);
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
	block->free_extra = NULL;
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
	if (block->free_extra)
		block->free_extra(block->extra);
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
	size_t uncompressed_size = sizeof(MapBlockData);
	char uncompressed_data[uncompressed_size];

	MapBlockData blockdata;
	ITERATE_MAPBLOCK {
		MapNode node = block->data[x][y][z];
		node.type = htobe32(node.type);
		blockdata[x][y][z] = node;
	}
	memcpy(uncompressed_data, blockdata, sizeof(MapBlockData));

	char compressed_data[uncompressed_size];

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	stream.avail_in = stream.avail_out = uncompressed_size;
    stream.next_in = (Bytef *) uncompressed_data;
    stream.next_out = (Bytef *) compressed_data;

    deflateInit(&stream, Z_BEST_COMPRESSION);
    deflate(&stream, Z_FINISH);
    deflateEnd(&stream);

	size_t compressed_size = stream.total_out;

	size_t size = sizeof(MapBlockHeader) + sizeof(MapBlockHeader) + compressed_size;
	char *data = malloc(size);
	*(MapBlockHeader *) data = htobe16(sizeof(MapBlockHeader) + compressed_size);
	*(MapBlockHeader *) (data + sizeof(MapBlockHeader)) = htobe16(uncompressed_size);
	memcpy(data + sizeof(MapBlockHeader) + sizeof(MapBlockHeader), compressed_data, compressed_size);

	*sizeptr = size;
	*dataptr = data;
}

bool map_deserialize_block(MapBlock *block, const char *data, size_t size)
{
	if (size < sizeof(MapBlockHeader))
		return false;

	MapBlockHeader uncompressed_size = be16toh(*(MapBlockHeader *) data);

	if (uncompressed_size < sizeof(MapBlockData))
		return false;

	char decompressed_data[uncompressed_size];

	z_stream stream;
	stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

	stream.avail_in = size - sizeof(MapBlockHeader);
    stream.next_in = (Bytef *) (data + sizeof(MapBlockHeader));
    stream.avail_out = uncompressed_size;
    stream.next_out = (Bytef *) decompressed_data;

    inflateInit(&stream);
    inflate(&stream, Z_NO_FLUSH);
    inflateEnd(&stream);

    if (stream.total_out < uncompressed_size)
		return false;

	MapBlockData blockdata;
	memcpy(blockdata, decompressed_data, sizeof(MapBlockData));

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
