#ifndef _CLIENT_NODE_H_
#define _CLIENT_NODE_H_

#include "client/object.h"
#include "client/texture.h"
#include "map.h"

typedef struct
{
	char *texture_path;
	Texture *texture;
	void (*render)(v3s32 pos, MapNode *node, Vertex3D *vertex);
} ClientNodeDefintion;

extern ClientNodeDefintion client_node_definitions[];
void client_node_init();

#endif
