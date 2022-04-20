#ifndef _NODE_H_
#define _NODE_H_

#include <stdbool.h>
#include <stddef.h>
#include "types.h"

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
	NODE_UNLOADED,      // Used for nodes in unloaded chunks
} NodeType;

struct TerrainNode;

typedef struct {
	bool solid;
	size_t data_size;
	unsigned long dig_class;
	struct {
		void (*create)(struct TerrainNode *node);
		void (*delete)(struct TerrainNode *node);
		void (*serialize)(Blob *buffer, void *data);
		void (*deserialize)(Blob *buffer, void *data);
	} callbacks;
} NodeDef;

extern NodeDef node_def[];

#endif
