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
	NODE_SNOW,
	NODE_UNLOADED,		// Used for nodes in unloaded blocks
} Node;

struct MapNode;

typedef struct
{
	bool visible;
	bool solid;
	void (*create)(struct MapNode *node);
	void (*serialize)(struct MapNode *node);
	void (*deserialize)(struct MapNode *node);
} NodeDefintion;

extern NodeDefintion node_definitions[];

#endif
