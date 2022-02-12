#include "types.h"
#include "map.h"
#include "node.h"
#include "util.h"
#include <stdio.h>

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
		.serialize = (void *) &HSLData_write,
		.deserialize = (void *) &HSLData_read,
	},
	// oak leaves
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &HSLData_write,
		.deserialize = (void *) &HSLData_read,
	},
	// pine wood
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &HSLData_write,
		.deserialize = (void *) &HSLData_read,
	},
	// pine leaves
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &HSLData_write,
		.deserialize = (void *) &HSLData_read,
	},
	// palm wood
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &HSLData_write,
		.deserialize = (void *) &HSLData_read,
	},
	// palm leaves
	{
		.solid = true,
		.data_size = sizeof(HSLData),
		.create = NULL,
		.delete = NULL,
		.serialize = (void *) &HSLData_write,
		.deserialize = (void *) &HSLData_read,
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
