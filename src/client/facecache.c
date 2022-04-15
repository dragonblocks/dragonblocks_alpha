#include <dragonstd/array.h>
#include <stdlib.h>
#include "client/facecache.h"

static Array positions;
static u32 radius;
static pthread_mutex_t mtx;

__attribute__((constructor)) static void facecache_init()
{
	array_ini(&positions, sizeof(v3s32), 1000);
	array_apd(&positions, &(v3s32) {0, 0, 0});
	pthread_mutex_init(&mtx, NULL);
	radius = 0;
}

__attribute__((destructor)) static void facecache_deinit()
{
	array_clr(&positions);
	pthread_mutex_destroy(&mtx);
}

static inline void facecache_calculate(s32 radius)
{
#define ADDPOS(a, b, c, va, vb, vc) \
	{ \
		v3s32 pos; \
		*(s32 *) ((char *) &pos + offsetof(v3s32, a)) = va; \
		*(s32 *) ((char *) &pos + offsetof(v3s32, b)) = vb; \
		*(s32 *) ((char *) &pos + offsetof(v3s32, c)) = vc; \
		array_apd(&positions, &pos); \
	}
#define SQUARES(a, b, c) \
	for (s32 va = -radius + 1; va < radius; va++) { \
		for (s32 vb = -radius + 1; vb < radius; vb++) { \
			ADDPOS(a, b, c, va, vb,  radius) \
			ADDPOS(a, b, c, va, vb, -radius) \
		} \
	}
	SQUARES(x, z, y)
	SQUARES(x, y, z)
	SQUARES(z, y, x)
#undef SQUARES
#define EDGES(a, b, c) \
	for (s32 va = -radius + 1; va < radius; va++) { \
		ADDPOS(a, b, c, va,  radius,  radius) \
		ADDPOS(a, b, c, va,  radius, -radius) \
		ADDPOS(a, b, c, va, -radius,  radius) \
		ADDPOS(a, b, c, va, -radius, -radius) \
	}
	EDGES(x, y, z)
	EDGES(z, x, y)
	EDGES(y, x, z)
#undef EDGES
	ADDPOS(x, y, z,  radius,  radius,  radius)
	ADDPOS(x, y, z,  radius,  radius, -radius)
	ADDPOS(x, y, z,  radius, -radius,  radius)
	ADDPOS(x, y, z,  radius, -radius, -radius)
	ADDPOS(x, y, z, -radius,  radius,  radius)
	ADDPOS(x, y, z, -radius,  radius, -radius)
	ADDPOS(x, y, z, -radius, -radius,  radius)
	ADDPOS(x, y, z, -radius, -radius, -radius)
#undef ADDPOS
}

v3s32 facecache_get(size_t i)
{
	pthread_mutex_lock(&mtx);

	if (positions.cap <= i) {
		positions.cap = i;
		array_rlc(&positions);
	}

	while (positions.siz <= i)
		facecache_calculate(++radius);

	v3s32 pos = ((v3s32 *) positions.ptr)[i];
	pthread_mutex_unlock(&mtx);
	return pos;
}

size_t facecache_count(u32 radius)
{
	size_t len = 1 + radius * 2;
	return len * len * len;
}
