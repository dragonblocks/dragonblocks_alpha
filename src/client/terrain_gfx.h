#ifndef _TERRAIN_GFX_H_
#define _TERRAIN_GFX_H_

#include "client/cube.h"
#include "terrain.h"

typedef struct {
	CubeVertex cube;
	f32 textureIndex;
	v3f32 color;
} __attribute__((packed)) TerrainVertex;

void terrain_gfx_init();
void terrain_gfx_deinit();
void terrain_gfx_update();
void terrain_gfx_make_chunk_model(TerrainChunk *chunk);

#endif
