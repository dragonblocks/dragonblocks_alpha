#include "map.h"
#include "node.h"
#include "util.h"

static void create_state_biome(MapNode *node)
{
	node->state.biome = (v3f32) {1.0f, 0.0f, 1.0f};
}

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
		.create = &create_state_biome,
		.serialize = NULL, // currently v3f is not serialized
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
};
