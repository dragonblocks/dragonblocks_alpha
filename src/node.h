#ifndef _NODE_H_
#define _NODE_H_

#include <stdbool.h>
#include <dragontype/number.h>

#define NODE_DEFINITION(type) ((type) < NODE_UNLOADED ? &node_definitions[NODE_UNKNOWN] : &node_definitions[(type)]);

typedef enum
{
	NODE_UNKNOWN,		// Used for unknown nodes received from server (caused by outdated clients)
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
	NODE_UNLOADED,		// Used for nodes in unloaded blocks
} Node;

struct MapNode;

typedef struct
{
	bool solid;
	size_t data_size;
	void (*create)(struct MapNode *node);
	void (*delete)(struct MapNode *node);
	void (*serialize)(struct MapNode *node, unsigned char **buffer, size_t *bufsiz);
	void (*deserialize)(struct MapNode *node, unsigned char *data, size_t size);
} NodeDefinition;

typedef struct {
	v3f32 color;
} HSLData;

extern NodeDefinition node_definitions[];

#endif
