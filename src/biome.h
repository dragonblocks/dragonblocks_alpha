#ifndef _BIOME_H_
#define _BIOME_H_

#include "types.h"

typedef enum
{
	SO_HEIGHT,
	SO_MOUNTAIN_FACTOR,
	SO_MOUNTAIN_HEIGHT,
	SO_BOULDER_CENTER,
	SO_BOULDER,
	SO_WETNESS,
	SO_TEXTURE_OFFSET_S,
	SO_TEXTURE_OFFSET_T,
} SeedOffset;

f64 get_wetness(v3s32 pos);

extern int seed;

#endif
