#include "client/client_item.h"
#include "client/interact.h"
#include "common/node.h"

static bool use_dig(__attribute__((unused)) ItemStack *stack)
{
	return interact_pointed.exists
		&& (node_def[interact_pointed.node].dig_class & item_def[stack->type].dig_class);
}

ClientItemDef client_item_def[COUNT_ITEM] = {
	// unknown
	{
		.mesh_path = ASSET_PATH "meshes/unknown.txt",
	},
	// none
	{},
	// pickaxe
	{
		.mesh_path = ASSET_PATH "meshes/pickaxe.txt",
		.use = &use_dig,
	},
	// axe
	{
		.mesh_path = ASSET_PATH "meshes/axe.txt",
		.use = &use_dig,
	},
	// shovel
	{
		.mesh_path = ASSET_PATH "meshes/shovel.txt",
		.use = &use_dig,
		.inventory_rot = -0.5,
	},
};

void client_item_init()
{
	for (ItemType i = 0; i < COUNT_ITEM; i++)
		if (client_item_def[i].mesh_path)
			mesh_load(&client_item_def[i].mesh, client_item_def[i].mesh_path, &client_item_def[i].mesh_extents);
}

void client_item_deinit()
{
	for (ItemType i = 0; i < COUNT_ITEM; i++)
		if (client_item_def[i].mesh_path)
			mesh_destroy(&client_item_def[i].mesh);
}

Mesh *client_item_mesh(ItemType type)
{
	return client_item_def[type].mesh_path ? &client_item_def[type].mesh : NULL;
}
