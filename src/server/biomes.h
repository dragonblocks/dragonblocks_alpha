#ifndef _BIOMES_H_
#define _BIOMES_H_

#include "common/perlin.h"
#include "common/terrain.h"
#include "types.h"

typedef enum {
	BIOME_MOUNTAIN,
	BIOME_OCEAN,
	BIOME_HILLS,
	COUNT_BIOME,
} Biome;

typedef struct {
	TerrainChunk *chunk;
	List *changed_chunks;
	void *chunk_data;
} BiomeArgsChunk;

typedef struct {
	v2s32 pos;
	f64 factor;
	void *row_data;
	void *chunk_data;
} BiomeArgsRow;

typedef struct {
	v2s32 pos;
	f64 factor;
	f32 height;
	void *row_data;
	void *chunk_data;
} BiomeArgsHeight;

typedef struct {
	v3s32 offset;
	v3s32 pos;
	s32 diff;
	f64 humidity;
	f64 temperature;
	f64 factor;
	TerrainChunk *chunk;
	List *changed_chunks;
	void *row_data;
	void *chunk_data;
} BiomeArgsGenerate;

typedef struct {
	f64 probability;
	SeedOffset offset;
	f64 threshold;
	bool snow;
	s32 (*height)(BiomeArgsHeight *args);
	NodeType (*generate)(BiomeArgsGenerate *args);
	size_t chunk_data_size;
	void (*before_chunk)(BiomeArgsChunk *args);
	void (*after_chunk)(BiomeArgsChunk *args);
	size_t row_data_size;
	void (*before_row)(BiomeArgsRow *args);
	void (*after_row)(BiomeArgsRow *args);
} BiomeDef;

extern BiomeDef biomes[];

Biome get_biome(v2s32 pos, f64 *factor);
NodeType ocean_get_node_at(v3s32 pos, s32 diff, void *_row_data);

#endif // _BIOMES_H_
