#include <math.h>
#include "server/biomes.h"
#include "server/mapgen.h"
#include "server/server_map.h"
#include "util.h"
#define NUM_BIOMES 3

static s32 height_mountain(v3s32 pos, f64 factor, s32 height)
{
	return pow((height + 96) * pow(((smooth2d(U32(pos.x) / 48.0, U32(pos.z) / 48.0, 0, seed + SO_MOUNTAIN_HEIGHT) + 1.0) * 256.0 + 128.0), factor), 1.0 / (factor + 1.0)) - 96;
}

static Node generate_mountain(unused v3s32 pos, s32 diff, unused f64 wetness, unused f64 temperature, unused f64 factor, unused MapBlock *block, unused List *changed_blocks)
{
	return diff <= 0 ? NODE_STONE : NODE_AIR;
}

typedef enum
{
	OL_BEACH_EDGE,
	OL_BEACH,
	OL_OCEAN,
	OL_DEEP_OCEAN,
	OL_COUNT
} OceanLevel;

static f64 ocean_level_start[OL_COUNT] = {
	0.0,
	0.2,
	0.3,
	0.8,
};

static OceanLevel get_ocean_level(f64 factor)
{
	if (factor >= ocean_level_start[OL_DEEP_OCEAN])
		return OL_DEEP_OCEAN;
	else if (factor >= ocean_level_start[OL_OCEAN])
		return OL_OCEAN;
	else if (factor >= ocean_level_start[OL_BEACH])
		return OL_BEACH;

	return OL_BEACH_EDGE;
}

static s32 mix(s32 a, s32 b, f64 f)
{
	return (a * (1.0 - f) + b * f);
}

static f64 get_ocean_level_factor(f64 factor, OceanLevel level)
{
	f64 start, end;
	start = ocean_level_start[level];
	end = ++level == OL_COUNT ? 1.0 : ocean_level_start[level];

	return (factor - start) / (end - start);
}

static s32 height_ocean(unused v3s32 pos, f64 factor, s32 height)
{
	switch (get_ocean_level(factor)) {
		case OL_BEACH_EDGE:
			return mix(height + 1, 0, get_ocean_level_factor(factor, OL_BEACH_EDGE));

		case OL_BEACH:
			return 0;

		case OL_OCEAN:
			return mix(0, -10, get_ocean_level_factor(factor, OL_OCEAN));

		case OL_DEEP_OCEAN:
			return mix(-10, -50, get_ocean_level_factor(factor, OL_DEEP_OCEAN));

		default:
			break;
	}

	return height;
}

static Node generate_ocean(unused v3s32 pos, s32 diff, unused f64 wetness, unused f64 temperature, unused f64 factor, unused MapBlock *block, unused List *changed_blocks)
{
	if (diff <= -5)
		return NODE_STONE;
	else if (diff <= 0)
		return NODE_SAND;
	else if (pos.y <= 0)
		return NODE_WATER;

	return NODE_AIR;
}

static s32 height_hills(unused v3s32 pos, unused f64 factor, s32 height)
{
	return height;
}

static Node generate_hills(v3s32 pos, s32 diff, unused f64 wetness, unused f64 temperature, unused f64 factor, unused MapBlock *block, List *changed_blocks)
{
	if (diff == 2 && smooth2d(U32(pos.x), U32(pos.z), 0, seed + SO_BOULDER_CENTER) > 0.999) {
		for (s8 bx = -1; bx <= 1; bx++) {
			for (s8 by = -1; by <= 1; by++) {
				for (s8 bz = -1; bz <= 1; bz++) {
					v3s32 bpos = {pos.x + bx, pos.y + by, pos.z + bz};
					if (smooth3d(bpos.x, bpos.y, bpos.z, 0, seed + SO_BOULDER) > 0.0)
						mapgen_set_node(bpos, map_node_create(NODE_STONE), MGS_BOULDERS, changed_blocks);
				}
			}
		}
	}

	if (diff <= -5)
		return NODE_STONE;
	else if (diff <= -1)
		return NODE_DIRT;
	else if (diff <= 0)
		return NODE_GRASS;

	return NODE_AIR;
}

static BiomeDef biomes[NUM_BIOMES] = {
	{
		.probability = 0.2,
		.offset = SO_MOUNTAIN,
		.height = &height_mountain,
		.generate = &generate_mountain,
	},
	{
		.probability = 0.2,
		.offset = SO_OCEAN,
		.height = &height_ocean,
		.generate = &generate_ocean,
	},
	{
		.probability = 1.0,
		.offset = SO_NONE,
		.height = &height_hills,
		.generate = &generate_hills,
	},
};

BiomeDef *get_biome(v3s32 pos, f64 *factor)
{
	for (int i = 0; i < NUM_BIOMES; i++) {
		BiomeDef *def = &biomes[i];
		f64 f = (smooth2d(U32(pos.x) / 1024.0, U32(pos.z) / 1024.0, 0, seed + def->offset) * 0.5 - 0.5 + def->probability) / def->probability;

		if (f > 0.0) {
			if (factor)
				*factor = f;
			return def;
		}
	}

	return NULL;
}
