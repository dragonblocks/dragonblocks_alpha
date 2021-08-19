#include "biome.h"
#include "perlin.h"

int seed = 1;

f64 get_wetness(v3s32 pos)
{
	return smooth2d((((u32) 1 << 31) + pos.x) / 128.0, (((u32) 1 << 31) + pos.z) / 128.0, 0, seed + SO_WETNESS) * 0.5 + 0.5;
}

f64 get_temperature(v3s32 pos)
{
	return smooth2d((((u32) 1 << 31) + pos.x) / 128.0, (((u32) 1 << 31) + pos.z) / 128.0, 0, seed + SO_TEMPERATURE) * 0.5 + 0.5 - (f64) (pos.y - 32.0) / 64.0;
}
