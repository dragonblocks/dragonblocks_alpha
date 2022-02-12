#ifndef _TREES_H_
#define _TREES_H_

#include <dragonstd/list.h>
#include "perlin.h"
#include "types.h"

#define NUM_TREES 3

typedef struct
{
	f32 spread;
	f32 probability;
	f32 area_probability;
	SeedOffset offset;
	SeedOffset area_offset;
	bool (*condition)(v3s32 pos, f64 humidity, f64 temperature, Biome biome, f64 factor, MapBlock *block, void *row_data, void *block_data);
	void (*generate)(v3s32 pos, List *changed_blocks);
} TreeDef;

extern TreeDef tree_definitions[];

#endif
