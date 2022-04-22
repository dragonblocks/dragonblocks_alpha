#include "node.h"
#include "server/server_item.h"
#include "server/server_node.h"
#include "server/server_terrain.h"
#include "server/tree_physics.h"

static void use_dig(__attribute__((unused)) ServerPlayer *player, ItemStack *stack, bool pointed, v3s32 pos)
{
	if (!pointed)
		return;

	v3s32 off;
	TerrainChunk *chunk = terrain_get_chunk_nodep(server_terrain, pos, &off, false);
	if (!chunk)
		return;
	TerrainChunkMeta *meta = chunk->extra;
	terrain_lock_chunk(chunk);

	TerrainNode *node = &chunk->data[off.x][off.y][off.z];

	if (!(node_def[node->type].dig_class & item_def[stack->type].dig_class)) {
		pthread_mutex_unlock(&chunk->mtx);
		return;
	}

	*node = server_node_create(NODE_AIR);
	meta->tgsb.raw.nodes[off.x][off.y][off.z] = STAGE_PLAYER;

	pthread_mutex_unlock(&chunk->mtx);

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
