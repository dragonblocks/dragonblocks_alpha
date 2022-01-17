#include "map.h"
#include "node.h"
#include "util.h"
#include <stdio.h>

static void serialize_hsl(MapNode *node, unsigned char **buffer, size_t *bufsiz)
{
	HSLData *node_data = node->data;
	buffer_write(buffer, bufsiz, (f32 []) {node_data->color.x, node_data->color.y, node_data->color.z}, sizeof(f32) * 3);
}

static void deserialize_hsl(MapNode *node, unsigned char *data, size_t size)
{
	HSLData *node_data = node->data;

	f32 *color = buffer_read(&data, &size, sizeof(f32) * 3);

	if (! color)
		return;

	*node_data = (HSLData) {.color = {color[0], color[1], color[2]}};
}

NodeDefinition node_definitions[NODE_UNLOADED] = {
	// unknown
	{
		.solid = true,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// air
	{
		.solid = false,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// grass
	{
		.solid = true,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// dirt
	{
		.solid = true,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// stone
	{
		.solid = true,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// snow
	{
		.solid = true,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// oak wood
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = &serialize_hsl,
		.deserialize = &deserialize_hsl,
	},
	// oak leaves
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = &serialize_hsl,
		.deserialize = &deserialize_hsl,
	},
	// pine wood
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = &serialize_hsl,
		.deserialize = &deserialize_hsl,
	},
	// pine leaves
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = &serialize_hsl,
		.deserialize = &deserialize_hsl,
	},
	// palm wood
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = &serialize_hsl,
		.deserialize = &deserialize_hsl,
	},
	// palm leaves
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = &serialize_hsl,
		.deserialize = &deserialize_hsl,
	},
	// sand
	{
		.solid = true,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// water
	{
		.solid = false,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// lava
	{
		.solid = false,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
	// vulcanostone
	{
		.solid = true,
		.data_size = 0,
		.create = NULL,
		.delete = NULL,
		.serialize = NULL,
		.deserialize = NULL,
	},
};
