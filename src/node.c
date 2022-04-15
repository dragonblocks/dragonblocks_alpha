#include "node.h"
#include "terrain.h"
#include "types.h"

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
		.data_size = sizeof(ColorData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &ColorData_write,
		.deserialize = (void *) &ColorData_read,
	},
	// oak leaves
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &ColorData_write,
		.deserialize = (void *) &ColorData_read,
	},
	// pine wood
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &ColorData_write,
		.deserialize = (void *) &ColorData_read,
	},
	// pine leaves
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &ColorData_write,
		.deserialize = (void *) &ColorData_read,
	},
	// palm wood
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &ColorData_write,
		.deserialize = (void *) &ColorData_read,
	},
	// palm leaves
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &ColorData_write,
		.deserialize = (void *) &ColorData_read,
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
