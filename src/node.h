#ifndef _NODE_H_
#define _NODE_H_

#include <stdbool.h>
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
	bool solid;
} NodeDefintion;

extern NodeDefintion node_definitions[];

#endif
