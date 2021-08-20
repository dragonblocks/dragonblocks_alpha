#ifndef _CLIENT_NODE_H_
#define _CLIENT_NODE_H_

#include "client/object.h"
#include "client/texture.h"
#include "map.h"

typedef enum
{
	NV_NONE,
	NV_TRANSPARENT,
	NV_SOLID,
} NodeVisibility;

typedef struct
{
	struct
	{
		char *paths[6];			// input
		int indices[6];			// input
		Texture *textures[6];	// output
	} tiles;
	NodeVisibility visibility;
	void (*render)(v3s32 pos, MapNode *node, Vertex3D *vertex, int f, int v);
} ClientNodeDefintion;

extern ClientNodeDefintion client_node_definitions[];
void client_node_init();

#endif
