#ifndef _BIOMES_H_
#define _BIOMES_H_

#include <dragontype/number.h>
#include "map.h"
#include "perlin.h"

typedef enum
{
	BIOME_MOUNTAIN,
	BIOME_OCEAN,
	BIOME_HILLS,
	BIOME_COUNT,
} Biome;

typedef struct
{
	f64 probability;
	SeedOffset offset;
	f64 threshold;
	bool snow;
	s32 (*height)(v2s32 pos, f64 factor, f32 height, void *row_data, void *block_data);
	Node (*generate)(v3s32 pos, s32 diff, f64 humidity, f64 temperature, f64 factor, MapBlock *block, List *changed_blocks, void *row_data, void *block_data);
	size_t block_data_size;
	void (*preprocess_block)(MapBlock *block, List *changed_blocks, void *block_data);
	size_t row_data_size;
	void (*preprocess_row)(v2s32 pos, f64 factor, void *row_data, void *block_data);
} BiomeDef;

extern BiomeDef biomes[];

Biome get_biome(v2s32 pos, f64 *factor);
Node ocean_get_node_at(v3s32 pos, s32 diff, void *row_data);

#endif
