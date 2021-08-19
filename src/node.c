#include "map.h"
#include "node.h"
#include "util.h"

NodeDefintion node_definitions[NODE_UNLOADED] = {
	// invalid
	{
		.visible = true,
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// air
	{
		.visible = false,
		.solid = false,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// grass
	{
		.visible = true,
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// dirt
	{
		.visible = true,
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// stone
	{
		.visible = true,
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// snow
	{
		.visible = true,
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// wood
	{
		.visible = true,
		.solid = true,
		.create = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
};
