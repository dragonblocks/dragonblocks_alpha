#ifndef _TREES_H_
#define _TREES_H_

#include <dragonstd/list.h>
#include <stdbool.h>
#include "perlin.h"
#include "terrain.h"
#include "types.h"

#define NUM_TREES 3

typedef struct {
	v3s32 pos;
	f64 humidity;
	f64 temperature;
	Biome biome;
	f64 factor;
	TerrainChunk *chunk;
	void *row_data;
	void *chunk_data;
} TreeArgsCondition;

typedef struct {
	f32 spread;
	f32 probability;
	f32 area_probability;
	SeedOffset offset;
	SeedOffset area_offset;
	bool (*condition)(TreeArgsCondition *args);
	void (*generate)(v3s32 pos, List *changed_chunks);
} TreeDef;

extern TreeDef tree_def[];

#endif // _TREES_H_
