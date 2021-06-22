#ifndef _CLIENTNODE_H_
#define _CLIENTNODE_H_

#include "node.h"

typedef struct
{
	char *texture_path;
	GLuint texture;
} ClientNodeDefintion;

extern ClientNodeDefintion client_node_definitions[];
void init_client_node_definitions();

#endif
