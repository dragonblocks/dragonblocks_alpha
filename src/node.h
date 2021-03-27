#ifndef _NODE_H_
#define _NODE_H_

#include "types.h"

typedef enum
{
	NODE_INVALID,		// Used for unknown nodes received from server (caused by outdated clients)
	NODE_AIR,
	NODE_GRASS,
	NODE_DIRT,
	NODE_STONE,
	NODE_UNLOADED,		// Used for nodes in unloaded blocks
} Node;

typedef struct
{
	bool visible;
	bool color_initialized;
	const char *color_str;
	v3f color;
} NodeDefintion;

v3f get_node_color(NodeDefintion *def);

extern NodeDefintion node_definitions[];

#endif
