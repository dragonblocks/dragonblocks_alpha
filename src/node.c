#include "dig.h"
#include "node.h"
#include "terrain.h"
#include "types.h"

NodeDef node_defs[NODE_UNLOADED] = {
	// unknown
	{
		.solid = true,
		.data_size = 0,
		.dig_class = DIG_NONE,
		.callbacks = {NULL},
	},
	// air
	{
		.solid = false,
		.data_size = 0,
		.dig_class = DIG_NONE,
		.callbacks = {NULL},
	},
	// grass
	{
		.solid = true,
		.data_size = 0,
		.dig_class = DIG_DIRT,
		.callbacks = {NULL},
	},
	// dirt
	{
		.solid = true,
		.data_size = 0,
		.dig_class = DIG_DIRT,
		.callbacks = {NULL},
	},
	// stone
	{
		.solid = true,
		.data_size = 0,
		.dig_class = DIG_STONE,
		.callbacks = {NULL},
	},
	// snow
	{
		.solid = true,
		.data_size = 0,
		.dig_class = DIG_DIRT,
		.callbacks = {NULL},
	},
	// oak wood
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.dig_class = DIG_WOOD,
		.callbacks = {
			.create = NULL,
			.delete = NULL,
			.serialize = (void *) &ColorData_write,
			.deserialize = (void *) &ColorData_read,
		},
	},
	// oak leaves
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.dig_class = DIG_NONE,
		.callbacks = {
			.create = NULL,
			.delete = NULL,
			.serialize = (void *) &ColorData_write,
			.deserialize = (void *) &ColorData_read,
		},
	},
	// pine wood
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.dig_class = DIG_WOOD,
		.callbacks = {
			.create = NULL,
			.delete = NULL,
			.serialize = (void *) &ColorData_write,
			.deserialize = (void *) &ColorData_read,
		},
	},
	// pine leaves
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.dig_class = DIG_NONE,
		.callbacks = {
			.create = NULL,
			.delete = NULL,
			.serialize = (void *) &ColorData_write,
			.deserialize = (void *) &ColorData_read,
		},
	},
	// palm wood
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.dig_class = DIG_WOOD,
		.callbacks = {
			.create = NULL,
			.delete = NULL,
			.serialize = (void *) &ColorData_write,
			.deserialize = (void *) &ColorData_read,
		},
	},
	// palm leaves
	{
		.solid = true,
		.data_size = sizeof(ColorData),
		.dig_class = DIG_NONE,
		.callbacks = {
			.create = NULL,
			.delete = NULL,
			.serialize = (void *) &ColorData_write,
			.deserialize = (void *) &ColorData_read,
		},
	},
	// sand
	{
		.solid = true,
		.data_size = 0,
		.dig_class = DIG_DIRT,
		.callbacks = {NULL},
	},
	// water
	{
		.solid = false,
		.data_size = 0,
		.dig_class = DIG_NONE,
		.callbacks = {NULL},
	},
	// lava
	{
		.solid = false,
		.data_size = 0,
		.dig_class = DIG_NONE,
		.callbacks = {NULL},
	},
	// vulcanostone
	{
		.solid = true,
		.data_size = 0,
		.dig_class = DIG_STONE,
		.callbacks = {NULL},
	},
};
