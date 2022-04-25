#ifndef _NODE_H_
#define _NODE_H_

#include <stdbool.h>
#include <stddef.h>
#include "types.h"

#define NODES_TREE case NODE_OAK_WOOD: case NODE_OAK_LEAVES: case NODE_PINE_WOOD: case NODE_PINE_LEAVES: case NODE_PALM_WOOD: case NODE_PALM_LEAVES:

typedef enum {
	NODE_UNKNOWN,       // Used for unknown nodes received from server (caused by outdated clients)
	NODE_AIR,
	NODE_GRASS,
	NODE_DIRT,
	NODE_STONE,
	NODE_SNOW,
	NODE_OAK_WOOD,
	NODE_OAK_LEAVES,
	NODE_PINE_WOOD,
	NODE_PINE_LEAVES,
	NODE_PALM_WOOD,
	NODE_PALM_LEAVES,
	NODE_SAND,
	NODE_WATER,
	NODE_LAVA,
	NODE_VULCANO_STONE,
	COUNT_NODE,      // Used for nodes in unloaded chunks
} NodeType;

struct TerrainNode;

typedef struct {
	bool solid;
	unsigned long dig_class;
} NodeDef;

extern NodeDef node_def[];

#endif
