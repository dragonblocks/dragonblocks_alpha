#include "node.h"
#include "server/server_item.h"
#include "server/server_terrain.h"

static void use_dig(__attribute__((unused)) ServerPlayer *player, ItemStack *stack, bool pointed, v3s32 pos)
{
	if (!pointed)
		return;

	NodeType node = terrain_get_node(server_terrain, pos).type;

	if (node == NODE_UNLOADED)
		return;

	if (!(node_def[node].dig_class & item_def[stack->type].dig_class))
		return;

	terrain_set_node(server_terrain, pos,
		terrain_node_create(NODE_AIR, (Blob) {0, NULL}),
		false, NULL);
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
