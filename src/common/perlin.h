#ifndef _PERLIN_H_
#define _PERLIN_H_

#include_next <perlin.h>
#include "types.h"

#define U32(x) (((u32) 1 << 31) + (x))

typedef enum {
	OFFSET_NONE,
	OFFSET_HEIGHT,
	OFFSET_MOUNTAIN,
	OFFSET_OCEAN,
	OFFSET_MOUNTAIN_HEIGHT,
	OFFSET_BOULDER,
	OFFSET_WETNESS,
	OFFSET_TEXTURE_OFFSET_S,
	OFFSET_TEXTURE_OFFSET_T,
	OFFSET_TEMPERATURE,
	OFFSET_VULCANO,
	OFFSET_VULCANO_HEIGHT,
	OFFSET_VULCANO_STONE,
	OFFSET_VULCANO_CRATER_TOP,
	OFFSET_HILLYNESS,
	OFFSET_VOXEL_PROCEDURAL,
	OFFSET_OAKTREE,
	OFFSET_OAKTREE_AREA,
	OFFSET_PINETREE,
	OFFSET_PINETREE_AREA,
	OFFSET_PINETREE_HEIGHT,
	OFFSET_PINETREE_BRANCH,
	OFFSET_PINETREE_BRANCH_DIR,
	OFFSET_PALMTREE,
	OFFSET_PALMTREE_AREA,
} SeedOffset;

extern s32 seed;

#endif // _PERLIN_H_
