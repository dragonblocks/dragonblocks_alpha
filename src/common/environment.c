#include "common/environment.h"
#include "common/perlin.h"

f64 get_humidity(v3s32 pos)
{
	return smooth2d(U32(pos.x) / 128.0, U32(pos.z) / 128.0, 0, seed + OFFSET_WETNESS) * 0.5 + 0.5;
}

f64 get_temperature(v3s32 pos)
{
	return smooth2d(U32(pos.x) / 128.0, U32(pos.z) / 128.0, 0, seed + OFFSET_TEMPERATURE) * 0.5 + 0.5 - (pos.y - 32.0) / 64.0;
}
