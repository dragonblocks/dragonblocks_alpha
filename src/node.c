#include "dig.h"
#include "node.h"
#include "terrain.h"
#include "types.h"

NodeDef node_def[COUNT_NODE] = {
	// unknown
	{
		.solid = true,
		.dig_class = DIG_NONE,
	},
	// air
	{
		.solid = false,
		.dig_class = DIG_NONE,
	},
	// grass
	{
		.solid = true,
		.dig_class = DIG_DIRT,
	},
	// dirt
	{
		.solid = true,
		.dig_class = DIG_DIRT,
	},
	// stone
	{
		.solid = true,
		.dig_class = DIG_STONE,
	},
	// snow
	{
		.solid = true,
		.dig_class = DIG_DIRT,
	},
	// oak wood
	{
		.solid = true,
		.dig_class = DIG_WOOD,
	},
	// oak leaves
	{
		.solid = true,
		.dig_class = DIG_LEAVES,
	},
	// pine wood
	{
		.solid = true,
		.dig_class = DIG_WOOD,
	},
	// pine leaves
	{
		.solid = true,
		.dig_class = DIG_LEAVES,
	},
	// palm wood
	{
		.solid = true,
		.dig_class = DIG_WOOD,
	},
	// palm leaves
	{
		.solid = true,
		.dig_class = DIG_LEAVES,
	},
	// sand
	{
		.solid = true,
		.dig_class = DIG_DIRT,
	},
	// water
	{
		.solid = false,
		.dig_class = DIG_NONE,
	},
	// lava
	{
		.solid = false,
		.dig_class = DIG_NONE,
	},
	// vulcanostone
	{
		.solid = true,
		.dig_class = DIG_STONE,
	},
};
