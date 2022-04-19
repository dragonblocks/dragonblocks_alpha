#ifndef _CLIENT_ITEM_H_
#define _CLIENT_ITEM_H_

#include "client/mesh.h"
#include "item.h"

typedef struct {
	const char *mesh_path;
	Mesh mesh;
} ClientItemDef;

extern ClientItemDef client_item_defs[];

void client_item_init();
void client_item_deinit();
Mesh *client_item_mesh(ItemType type);

#endif // _CLIENT_ITEM_H_
