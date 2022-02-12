#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "map.h"
#include "util.h"

Map *map_create(MapCallbacks callbacks)
{
	Map *map = malloc(sizeof(Map));
	pthread_rwlock_init(&map->rwlck, NULL);
	pthread_rwlock_init(&map->cached_rwlck, NULL);
	map->sectors = bintree_create(sizeof(v2s32), NULL);
	map->cached = NULL;
	map->callbacks = callbacks;
	return map;
}

static void free_block(BintreeNode *node, void *arg)
{
	Map *map = arg;

	if (map->callbacks.delete_block)
		map->callbacks.delete_block(node->value);

	map_free_block(node->value);
}

static void free_sector(BintreeNode *node, void *arg)
{
	MapSector *sector = node->value;

	bintree_clear(&sector->blocks, &free_block, arg);
	pthread_rwlock_destroy(&sector->rwlck);
	free(sector);
}

void map_delete(Map *map)
{
	pthread_rwlock_destroy(&map->rwlck);
	pthread_rwlock_destroy(&map->cached_rwlck);
	bintree_clear(&map->sectors, &free_sector, map);
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
		sector->blocks = bintree_create(sizeof(s32), NULL);

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

	if (cached && v3s32_equals(cached->pos, pos))
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

		pthread_mutex_lock(&block->mtx);
		if (map->callbacks.get_block && ! map->callbacks.get_block(block, create)) {
			pthread_mutex_unlock(&block->mtx);
			block = NULL;
		} else {
			pthread_mutex_unlock(&block->mtx);
			pthread_rwlock_wrlock(&map->cached_rwlck);
			map->cached = block;
			pthread_rwlock_unlock(&map->cached_rwlck);
		}
	} else if (create) {
		bintree_add_node(&sector->blocks, nodeptr, &pos.y, block = map_allocate_block(pos));

		if (map->callbacks.create_block)
			map->callbacks.create_block(block);
	}

	pthread_rwlock_unlock(&sector->rwlck);

	return block;
}

MapBlock *map_allocate_block(v3s32 pos)
{
	MapBlock *block = malloc(sizeof(MapBlock));
	block->pos = pos;
	block->extra = NULL;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&block->mtx, &attr);

	ITERATE_MAPBLOCK block->data[x][y][z] = map_node_create(NODE_UNKNOWN, (Blob) {0, NULL});

	return block;
}

void map_free_block(MapBlock *block)
{
	ITERATE_MAPBLOCK map_node_delete(block->data[x][y][z]);

	pthread_mutex_destroy(&block->mtx);
	free(block);
}

Blob map_serialize_block(MapBlock *block)
{
	SerializedMapBlock block_data;

	ITERATE_MAPBLOCK {
		MapNode *node = &block->data[x][y][z];
		SerializedMapNode *node_data = &block_data.raw.nodes[x][y][z];

		*node_data = (SerializedMapNode) {
			.type = node->type,
			.data = {
				.siz = 0,
				.data = NULL,
			},
		};

		NodeDefinition *def = &node_definitions[node->type];

		if (def->serialize)
			def->serialize(&node_data->data, node->data);
	}

	Blob buffer = {0, NULL};
	SerializedMapBlock_write(&buffer, &block_data);
	SerializedMapBlock_free(&block_data);

	return buffer;
}

bool map_deserialize_block(MapBlock *block, Blob buffer)
{
	// it's important to copy Blobs that have been malloc'd before reading from them
	// because reading from a Blob modifies its data and size pointer,
	// but does not free anything
	SerializedMapBlock block_data = {0};
	bool success = SerializedMapBlock_read(&buffer, &block_data);

	if (success) ITERATE_MAPBLOCK
		block->data[x][y][z] = map_node_create(block_data.raw.nodes[x][y][z].type, block_data.raw.nodes[x][y][z].data);

	SerializedMapBlock_free(&block_data);
	return success;
}

v3s32 map_node_to_block_pos(v3s32 pos, v3u8 *offset)
{
	if (offset)
		*offset = (v3u8) {(u32) pos.x % MAPBLOCK_SIZE, (u32) pos.y % MAPBLOCK_SIZE, (u32) pos.z % MAPBLOCK_SIZE};
	return (v3s32) {floor((double) pos.x / (double) MAPBLOCK_SIZE), floor((double) pos.y / (double) MAPBLOCK_SIZE), floor((double) pos.z / (double) MAPBLOCK_SIZE)};
}

MapNode map_get_node(Map *map, v3s32 pos)
{
	v3u8 offset;
	v3s32 blockpos = map_node_to_block_pos(pos, &offset);
	MapBlock *block = map_get_block(map, blockpos, false);
	if (! block)
		return map_node_create(NODE_UNLOADED, (Blob) {0, NULL});
	return block->data[offset.x][offset.y][offset.z];
}

void map_set_node(Map *map, v3s32 pos, MapNode node, bool create, void *arg)
{
	v3u8 offset;
	MapBlock *block = map_get_block(map, map_node_to_block_pos(pos, &offset), create);
	if (block) {
		pthread_mutex_lock(&block->mtx);
		if (! map->callbacks.set_node || map->callbacks.set_node(block, offset, &node, arg)) {
			block->data[offset.x][offset.y][offset.z] = node;
			if (map->callbacks.after_set_node)
				map->callbacks.after_set_node(block, offset, arg);
		} else {
			map_node_delete(node);
		}
		pthread_mutex_unlock(&block->mtx);
	}
}

MapNode map_node_create(Node type, Blob buffer)
{
	if (type >= NODE_UNLOADED)
		type = NODE_UNKNOWN;

	NodeDefinition *def = &node_definitions[type];

	MapNode node;
	node.type = type;
	node.data = def->data_size ? malloc(def->data_size) : NULL;

	if (def->create)
		def->create(&node);

	if (def->deserialize)
		def->deserialize(&buffer, node.data);

	return node;
}

void map_node_delete(MapNode node)
{
	NodeDefinition *def = &node_definitions[node.type];

	if (def->delete)
		def->delete(&node);

	if (node.data)
		free(node.data);
}
