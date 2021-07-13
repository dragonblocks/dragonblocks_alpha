#ifndef _CLIENTNODE_H_
#define _CLIENTNODE_H_

#include "map.h"
#include "mesh.h"
#include "texture.h"

typedef struct
{
	char *texture_path;
	Texture *texture;
	void (*render)(MapNode *node, Vertex *vertex);
} ClientNodeDefintion;

extern ClientNodeDefintion client_node_definitions[];
void init_client_node_definitions();

#endif
