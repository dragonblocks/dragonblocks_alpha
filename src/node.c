#include "map.h"
#include "node.h"
#include "util.h"

NodeDefintion node_definitions[NODE_UNLOADED] = {
	// invalid
	{
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// air
	{
		.solid = false,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// grass
	{
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// dirt
	{
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// stone
	{
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// snow
	{
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// wood
	{
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// sand
	{
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// water
	{
		.solid = false,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
};
