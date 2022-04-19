#include "client/client_item.h"

ClientItemDef client_item_defs[COUNT_ITEM] = {
	// unknown
	{
		.mesh_path = RESSOURCE_PATH "meshes/unknown.txt",
		.mesh = {0},
	},
	// none
	{
		.mesh_path = NULL,
		.mesh = {0},
	},
	// pickaxe
	{
		.mesh_path = RESSOURCE_PATH "meshes/pickaxe.txt",
		.mesh = {0},
	},
	// axe
	{
		.mesh_path = RESSOURCE_PATH "meshes/axe.txt",
		.mesh = {0},
	},
};

void client_item_init()
{
	for (ItemType i = 0; i < COUNT_ITEM; i++)
		if (client_item_defs[i].mesh_path)
			mesh_load(&client_item_defs[i].mesh, client_item_defs[i].mesh_path);
}

void client_item_deinit()
{
	for (ItemType i = 0; i < COUNT_ITEM; i++)
		if (client_item_defs[i].mesh_path)
			mesh_destroy(&client_item_defs[i].mesh);
}

Mesh *client_item_mesh(ItemType type)
{
	return client_item_defs[type].mesh_path ? &client_item_defs[type].mesh : NULL;
}
