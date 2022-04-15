#include <math.h>
#include "server/biomes.h"
#include "server/server_terrain.h"
#include "server/terrain_gen.h"

Biome get_biome(v2s32 pos, f64 *factor)
{
	for (Biome i = 0; i < COUNT_BIOME; i++) {
		BiomeDef *def = &biomes[i];
		f64 f = def->probability == 1.0 ? 1.0
			: (smooth2d(U32(pos.x) / def->threshold, U32(pos.y) / def->threshold, 0, seed + def->offset) * 0.5 - 0.5 + def->probability) / def->probability;

		if (f > 0.0) {
			if (factor)
				*factor = f;
			return i;
		}
	}

	return COUNT_BIOME;
}

// mountain biome

static s32 height_mountain(BiomeArgsHeight *args)
{
	return pow((args->height + 96) * pow(((smooth2d(U32(args->pos.x) / 48.0, U32(args->pos.y) / 48.0, 0, seed + SO_MOUNTAIN_HEIGHT) + 1.0) * 256.0 + 128.0), args->factor), 1.0 / (args->factor + 1.0)) - 96;
}

static NodeType generate_mountain(BiomeArgsGenerate *args)
{
	return args->diff <= 0 ? NODE_STONE : NODE_AIR;
}

// ocean biome

typedef enum {
	OCEAN_EDGE,
	OCEAN_BEACH,
	OCEAN_MAIN,
	OCEAN_DEEP,
	COUNT_OCEAN
} OceanLevel;

static f64 ocean_level_start[COUNT_OCEAN] = {
	0.0,
	0.1,
	0.2,
	0.5,
};

typedef struct {
	bool has_vulcano;
	v2s32 vulcano_pos;
} OceanChunkData;

typedef struct {
	bool vulcano;
	bool vulcano_crater;
	s32 vulcano_height;
	s32 vulcano_crater_top;
	NodeType vulcano_stone;
} OceanRowData;

static const f64 vulcano_radius = 256.0;
static const f64 vulcano_chunk_offset = vulcano_radius * 2.0 / CHUNK_SIZE;

static OceanLevel get_ocean_level(f64 factor)
{
	if (factor >= ocean_level_start[OCEAN_DEEP])
		return OCEAN_DEEP;
	else if (factor >= ocean_level_start[OCEAN_MAIN])
		return OCEAN_MAIN;
	else if (factor >= ocean_level_start[OCEAN_BEACH])
		return OCEAN_BEACH;

	return OCEAN_EDGE;
}

static f64 get_ocean_level_factor(f64 factor, OceanLevel level)
{
	f64 start, end;
	start = ocean_level_start[level];
	end = ++level == COUNT_OCEAN ? 1.0 : ocean_level_start[level];

	return (factor - start) / (end - start);
}

static bool is_vulcano(v2s32 pos)
{
	f64 factor;

	return noise2d(pos.x, pos.y, 0, seed + SO_VULCANO) > 0.0
		&& get_biome((v2s32) {pos.x * CHUNK_SIZE, pos.y * CHUNK_SIZE}, &factor) == BIOME_OCEAN
		&& get_ocean_level(factor) == OCEAN_DEEP;
}

static bool find_near_vulcano(v2s32 pos, v2s32 *result)
{
	f64 x = pos.x / vulcano_chunk_offset;
	f64 z = pos.y / vulcano_chunk_offset;

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

static f64 distance(v2s32 a, v2s32 b)
{
	return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

static s32 calculate_ocean_floor(f64 factor, s32 height)
{
	switch (get_ocean_level(factor)) {
		case OCEAN_EDGE:
			return f64_mix(height + 1, 0, pow(get_ocean_level_factor(factor, OCEAN_EDGE), 0.8));

		case OCEAN_BEACH:
			return 0;

		case OCEAN_MAIN:
			return f64_mix(0, -10, pow(get_ocean_level_factor(factor, OCEAN_MAIN), 0.5));

		case OCEAN_DEEP:
			return f64_mix(-10, -50, pow(get_ocean_level_factor(factor, OCEAN_DEEP), 0.5));

		default:
			break;
	}

	return height;
}

static void chunk_ocean(BiomeArgsChunk *args)
{
	OceanChunkData *chunk_data = args->chunk_data;
	v2s32 vulcano_pos;

	if ((chunk_data->has_vulcano = find_near_vulcano((v2s32) {args->chunk->pos.x, args->chunk->pos.z}, &vulcano_pos)))
		chunk_data->vulcano_pos = (v2s32) {vulcano_pos.x * CHUNK_SIZE, vulcano_pos.y * CHUNK_SIZE};
}

static void row_ocean(BiomeArgsRow *args)
{
	OceanChunkData *chunk_data = args->chunk_data;
	OceanRowData *row_data = args->row_data;

	row_data->vulcano = false;

	if (chunk_data->has_vulcano) {
		f64 dist = distance(args->pos, chunk_data->vulcano_pos);

		if (dist < vulcano_radius) {
			f64 crater_factor = pow(asin(1.0 - dist / vulcano_radius), 2.0);
			f64 vulcano_height = (pnoise2d(U32(args->pos.x) / 100.0, U32(args->pos.y) / 100.0, 0.2, 2, seed + SO_VULCANO_HEIGHT) * 0.5 + 0.5) * 128.0 * crater_factor + 1.0 - 30.0;
			bool is_crater = vulcano_height > 0;

			if (!is_crater)
				vulcano_height = f64_min(vulcano_height + 5.0, 0.0);

			if (vulcano_height < 0)
				vulcano_height *= 2.0;

			row_data->vulcano = true;
			row_data->vulcano_crater = is_crater;
			row_data->vulcano_height = floor(vulcano_height + 0.5);
			row_data->vulcano_crater_top = 50 + floor((pnoise2d(U32(args->pos.x) / 3.0, U32(args->pos.y) / 3.0, 0.0, 1, seed + SO_VULCANO_CRATER_TOP) * 0.5 + 0.5) * 3.0 + 0.5);
			row_data->vulcano_stone = is_crater
				? ((pnoise2d(U32(args->pos.x) / 16.0, U32(args->pos.y) / 16.0, 0.85, 3, seed + SO_VULCANO_STONE) * 0.5 + 0.5) * crater_factor > 0.4
					? NODE_VULCANO_STONE
					: NODE_STONE)
				: NODE_SAND;
		}
	}
}

static s32 height_ocean(BiomeArgsHeight *args)
{
	OceanRowData *row_data = args->row_data;

	s32 ocean_floor = calculate_ocean_floor(args->factor, args->height);
	return row_data->vulcano ? f64_max(ocean_floor, row_data->vulcano_height) : ocean_floor;
}

NodeType ocean_get_node_at(v3s32 pos, s32 diff, void *_row_data)
{
	OceanRowData *row_data = _row_data;

	if (row_data->vulcano && row_data->vulcano_crater) {
		if (diff <= -5)
			return pos.y <= 45 ? NODE_LAVA : NODE_AIR;
		else if (diff <= 0)
			return pos.y <= row_data->vulcano_crater_top ? row_data->vulcano_stone : NODE_AIR;
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

static NodeType generate_ocean(BiomeArgsGenerate *args)
{
	return ocean_get_node_at(args->pos, args->diff, args->row_data);
}

// hills biome

static bool boulder_touching_ground(v3s32 pos, s32 diff)
{
	for (s32 dir = diff > 0 ? -1 : +1; dir > 0 ? diff <= 0 : diff >= 0; pos.y += dir, diff += dir) {
		if (smooth3d(U32(pos.x) / 12.0, U32(pos.y) / 6.0, U32(pos.z) / 12.0, 0, seed + SO_BOULDER) < 0.8)
			return false;
	}

	return true;
}

static s32 height_hills(BiomeArgsHeight *args)
{
	return args->height;
}

static NodeType generate_hills(BiomeArgsGenerate *args)
{
	if (boulder_touching_ground(args->pos, args->diff))
		return NODE_STONE;

	if (args->diff <= -5)
		return NODE_STONE;
	else if (args->diff <= -1)
		return NODE_DIRT;
	else if (args->diff <= 0)
		return NODE_GRASS;

	return NODE_AIR;
}

BiomeDef biomes[COUNT_BIOME] = {
	{
		.probability = 0.2,
		.offset = SO_MOUNTAIN,
		.threshold = 1024.0,
		.snow = true,
		.height = &height_mountain,
		.generate = &generate_mountain,
		.chunk_data_size = 0,
		.chunk = NULL,
		.row_data_size = 0,
		.row = NULL,
	},
	{
		.probability = 0.2,
		.offset = SO_OCEAN,
		.threshold = 2048.0,
		.snow = false,
		.height = &height_ocean,
		.generate = &generate_ocean,
		.chunk_data_size = sizeof(OceanChunkData),
		.chunk = &chunk_ocean,
		.row_data_size = sizeof(OceanRowData),
		.row = &row_ocean,
	},
	{
		.probability = 1.0,
		.offset = SO_NONE,
		.threshold = 0.0,
		.snow = true,
		.height = &height_hills,
		.generate = &generate_hills,
		.chunk_data_size = 0,
		.chunk = NULL,
		.row_data_size = 0,
		.row = NULL,
	},
};
