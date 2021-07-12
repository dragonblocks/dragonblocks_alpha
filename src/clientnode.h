#ifndef _CLIENTNODE_H_
#define _CLIENTNODE_H_

#include "node.h"
#include "texture.h"

typedef struct
{
	char *texture_path;
	Texture *texture;
} ClientNodeDefintion;

extern ClientNodeDefintion client_node_definitions[];
void init_client_node_definitions();

#endif
