#include <math.h>
#include "server/biomes.h"
#include "server/mapgen.h"
#include "server/server_map.h"
#include "util.h"

Biome get_biome(v2s32 pos, f64 *factor)
{
	for (Biome i = 0; i < BIOME_COUNT; i++) {
		BiomeDef *def = &biomes[i];
		f64 f = def->probability == 1.0 ? 1.0 : (smooth2d(U32(pos.x) / def->threshold, U32(pos.y) / def->threshold, 0, seed + def->offset) * 0.5 - 0.5 + def->probability) / def->probability;

		if (f > 0.0) {
			if (factor)
				*factor = f;
			return i;
		}
	}

	return BIOME_COUNT;
}

// mountain biome

static s32 height_mountain(v2s32 pos, f64 factor, s32 height, unused void *row_data, unused void *block_data)
{
	return pow((height + 96) * pow(((smooth2d(U32(pos.x) / 48.0, U32(pos.y) / 48.0, 0, seed + SO_MOUNTAIN_HEIGHT) + 1.0) * 256.0 + 128.0), factor), 1.0 / (factor + 1.0)) - 96;
}

static Node generate_mountain(unused v3s32 pos, s32 diff, unused f64 wetness, unused f64 temperature, unused f64 factor, unused MapBlock *block, unused List *changed_blocks, unused void *row_data, unused void *block_data)
{
	return diff <= 0 ? NODE_STONE : NODE_AIR;
}

// ocean biome

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
	0.1,
	0.2,
	0.5,
};

typedef struct
{
	bool has_vulcano;
	v2s32 vulcano_pos;
} OceanBlockData;

typedef struct
{
	bool vulcano;
	bool vulcano_crater;
	s32 vulcano_height;
	s32 vulcano_crater_top;
	Node vulcano_stone;
} OceanRowData;

static const f64 vulcano_radius = 256.0;
static const f64 vulcano_block_offset = vulcano_radius * 2.0 / MAPBLOCK_SIZE;

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

static f64 get_ocean_level_factor(f64 factor, OceanLevel level)
{
	f64 start, end;
	start = ocean_level_start[level];
	end = ++level == OL_COUNT ? 1.0 : ocean_level_start[level];

	return (factor - start) / (end - start);
}

static bool is_vulcano(v2s32 pos)
{
	f64 factor;
	return smooth2d(U32(pos.x), U32(pos.y), 0, seed + SO_VULCANO) > 0.0 && get_biome((v2s32) {pos.x * MAPBLOCK_SIZE, pos.y * MAPBLOCK_SIZE}, &factor) == BIOME_OCEAN && get_ocean_level(factor) == OL_DEEP_OCEAN;
}

static bool find_near_vulcano(v2s32 pos, v2s32 *result)
{
	f64 x = pos.x / vulcano_block_offset;
	f64 z = pos.y / vulcano_block_offset;

	s32 lx, lz;
	lx = floor(x);
	lz = floor(z);

	s32 hx, hz;
	hx = ceil(x);
	hz = ceil(z);

	for (s32 ix = lx; ix <= hx; ix++) {
		for (s32 iz = lz; iz <= hz; iz++) {
			v2s32 vulcano_pos = {ix * 32, iz * 32};

			if (is_vulcano(vulcano_pos)) {
				*result = vulcano_pos;
				return true;
			}
		}
	}

	return false;
}

static f64 mix(f64 a, f64 b, f64 f)
{
	return (a * (1.0 - f) + b * f);
}

static inline f64 min(f64 a, f64 b)
{
	return a < b ? a : b;
}

static inline f64 max(f64 a, f64 b)
{
	return a > b ? a : b;
}

static f64 distance(v2s32 a, v2s32 b)
{
	return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

static s32 calculate_ocean_floor(f64 factor, s32 height)
{
	switch (get_ocean_level(factor)) {
		case OL_BEACH_EDGE:
			return mix(height + 1, 0, pow(get_ocean_level_factor(factor, OL_BEACH_EDGE), 0.8));

		case OL_BEACH:
			return 0;

		case OL_OCEAN:
			return mix(0, -10, pow(get_ocean_level_factor(factor, OL_OCEAN), 0.5));

		case OL_DEEP_OCEAN:
			return mix(-10, -50, pow(get_ocean_level_factor(factor, OL_DEEP_OCEAN), 0.5));

		default:
			break;
	}

	return height;
}

static void preprocess_block_ocean(MapBlock *block, unused List *changed_blocks, void *block_data)
{
	OceanBlockData *data = block_data;

	v2s32 vulcano_pos;
	if ((data->has_vulcano = find_near_vulcano((v2s32) {block->pos.x, block->pos.z}, &vulcano_pos)))
		data->vulcano_pos = (v2s32) {vulcano_pos.x * MAPBLOCK_SIZE, vulcano_pos.y * MAPBLOCK_SIZE};
}

static void preprocess_row_ocean(v2s32 pos, unused s32 height, unused f64 factor, void *row_data, void *block_data)
{
	OceanRowData *rdata = row_data;
	OceanBlockData *bdata = block_data;
	rdata->vulcano = false;

	if (bdata->has_vulcano) {
		f64 dist = distance(pos, bdata->vulcano_pos);

		if (dist < vulcano_radius) {
			f64 crater_factor = pow(asin(1.0 - dist / vulcano_radius), 2.0);
			f64 vulcano_height = (pnoise2d(U32(pos.x) / 100.0, U32(pos.y) / 100.0, 0.2, 2, seed + SO_VULCANO_HEIGHT) * 0.5 + 0.5) * 128.0 * crater_factor + 1.0 - 30.0;
			bool is_crater = vulcano_height > 0;

			if (! is_crater)
				vulcano_height = min(vulcano_height + 5.0, 0.0);

			if (vulcano_height < 0)
				vulcano_height *= 2.0;

			rdata->vulcano = true;
			rdata->vulcano_crater = is_crater;
			rdata->vulcano_height = floor(vulcano_height + 0.5);
			rdata->vulcano_crater_top = 50 + floor((pnoise2d(U32(pos.x) / 3.0, U32(pos.y) / 3.0, 0.0, 1, seed + SO_VULCANO_CRATER_TOP) * 0.5 + 0.5) * 3.0 + 0.5);
			rdata->vulcano_stone = is_crater ? ((pnoise2d(U32(pos.x) / 16.0, U32(pos.y) / 16.0, 0.85, 3, seed + SO_VULCANO_STONE) * 0.5 + 0.5) * crater_factor > 0.4 ? NODE_VULCANO_STONE : NODE_STONE) : NODE_SAND;
		}
	}
}

static s32 height_ocean(unused v2s32 pos, f64 factor, s32 height, void *row_data, unused void *block_data)
{
	OceanRowData *rdata = row_data;
	s32 ocean_floor = calculate_ocean_floor(factor, height);

	return rdata->vulcano ? max(ocean_floor, rdata->vulcano_height) : ocean_floor;
}

static Node generate_ocean(v3s32 pos, s32 diff, unused f64 wetness, unused f64 temperature, unused f64 factor, unused MapBlock *block, unused List *changed_blocks, void *row_data, unused void *block_data)
{
	OceanRowData *rdata = row_data;

	if (rdata->vulcano && rdata->vulcano_crater) {
		if (diff <= -5)
			return pos.y <= 45 ? NODE_LAVA : NODE_AIR;
		else if (diff <= 0)
			return pos.y <= rdata->vulcano_crater_top ? rdata->vulcano_stone : NODE_AIR;
		else
			return NODE_AIR;
	} else {
		if (diff <= -5)
			return NODE_STONE;
		else if (diff <= 0)
			return NODE_SAND;
		else if (pos.y <= 0)
			return NODE_WATER;
	}

	return NODE_AIR;
}

// hills biome

static s32 height_hills(unused v2s32 pos, unused f64 factor, s32 height, unused void *row_data, unused void *block_data)
{
	return height;
}

static Node generate_hills(v3s32 pos, s32 diff, unused f64 wetness, unused f64 temperature, unused f64 factor, unused MapBlock *block, List *changed_blocks, unused void *row_data, unused void *block_data)
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

BiomeDef biomes[BIOME_COUNT] = {
	{
		.probability = 0.2,
		.offset = SO_MOUNTAIN,
		.threshold = 1024.0,
		.snow = true,
		.height = &height_mountain,
		.generate = &generate_mountain,
		.block_data_size = 0,
		.preprocess_block = NULL,
		.row_data_size = 0,
		.preprocess_row = NULL,
	},
	{
		.probability = 0.2,
		.offset = SO_OCEAN,
		.threshold = 2048.0,
		.snow = false,
		.height = &height_ocean,
		.generate = &generate_ocean,
		.block_data_size = sizeof(OceanBlockData),
		.preprocess_block = &preprocess_block_ocean,
		.row_data_size = sizeof(OceanRowData),
		.preprocess_row = &preprocess_row_ocean,

	},
	{
		.probability = 1.0,
		.offset = SO_NONE,
		.threshold = 0.0,
		.snow = true,
		.height = &height_hills,
		.generate = &generate_hills,
		.block_data_size = 0,
		.preprocess_block = NULL,
		.row_data_size = 0,
		.preprocess_row = NULL,
	},
};
