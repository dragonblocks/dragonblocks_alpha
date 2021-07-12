#include "node.h"
#include "util.h"

NodeDefintion node_definitions[NODE_UNLOADED] = {
	// invalid
	{
		.visible = true,
		.solid = true,
	},
	// air
	{
		.visible = false,
		.solid = false,
	},
	// grass
	{
		.visible = true,
		.solid = true,
	},
	// dirt
	{
		.visible = true,
		.solid = true,
	},
	// stone
	{
		.visible = true,
		.solid = true,
	},
	// snow
	{
		.visible = true,
		.solid = true,
	},
};
