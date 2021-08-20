#ifndef _BIOMES_H_
#define _BIOMES_H_

#include "types.h"
#include "map.h"
#include "perlin.h"

typedef struct
{
	f64 probability;
	SeedOffset offset;
	s32 (*height)(v3s32 pos, f64 factor, s32 height);
	Node (*generate)(v3s32 pos, s32 diff, f64 wetness, f64 temperature, f64 factor, MapBlock *block, List *changed_blocks);
} BiomeDef;

BiomeDef *get_biome(v3s32 pos, f64 *factor);

#endif
