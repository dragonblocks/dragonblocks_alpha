#include <assert.h>
#include "common/node.h"
#include "server/server_item.h"
#include "server/server_node.h"
#include "server/server_terrain.h"
#include "server/tree_physics.h"

static void use_dig(__attribute__((unused)) ServerPlayer *player, ItemStack *stack, bool pointed, v3s32 pos)
{
	if (!pointed)
		return;

	v3s32 offset;
	TerrainChunk *chunk = terrain_get_chunk_nodep(server_terrain, pos, &offset, CHUNK_MODE_PASSIVE);
	if (!chunk)
		return;
	TerrainChunkMeta *meta = chunk->extra;
	assert(pthread_rwlock_wrlock(&chunk->lock) == 0);

	TerrainNode *node = &chunk->data[offset.x][offset.y][offset.z];

	if (!(node_def[node->type].dig_class & item_def[stack->type].dig_class)) {
		pthread_rwlock_unlock(&chunk->lock);
		return;
	}

	*node = server_node_create(NODE_AIR);
	meta->tgsb.raw.nodes[offset.x][offset.y][offset.z] = STAGE_PLAYER;

	pthread_rwlock_unlock(&chunk->lock);

	server_terrain_lock_and_send_chunk(chunk);

	// destroy trees if they have no connection to ground
	// todo: run in seperate thread to not block client connection
	tree_physics_check(pos);
}

ServerItemDef server_item_def[COUNT_ITEM] = {
	// unknown
	{
		.use = NULL,
	},
	// none
	{
		.use = NULL,
	},
	// pickaxe
	{
		.use = &use_dig,
	},
	// axe
	{
		.use = &use_dig,
	},
	// shovel
	{
		.use = &use_dig,
	},
};
