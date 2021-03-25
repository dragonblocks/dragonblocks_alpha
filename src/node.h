#ifndef _NODE_H_
#define _NODE_H_

#include <linmath.h/linmath.h>

typedef enum
{
	NODE_INVALID,		// Used for invalid nodes received from server (caused by outdated clients)
	NODE_AIR,
	NODE_GRASS,
	NODE_DIRT,
	NODE_STONE,
	NODE_UNLOADED,		// Used for nodes in unloaded blocks
} Node;

typedef struct
{
	bool visible;
	vec3 color;
} NodeDefintion;

extern NodeDefintion node_definitions[];

#endif
