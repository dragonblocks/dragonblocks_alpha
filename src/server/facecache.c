#include <stdlib.h>
#include "server/facecache.h"
#include "array.h"

static struct
{
	Array positions;
	u32 size;
	pthread_mutex_t mtx;
} facecache;

__attribute((constructor)) static void face_cache_init()
{
	facecache.size = 0;
	facecache.positions = array_create(sizeof(v3s32));
	v3s32 pos = {0, 0, 0};
	array_append(&facecache.positions, &pos);
	pthread_mutex_init(&facecache.mtx, NULL);
}

__attribute((destructor)) void face_cache_deinit()
{
	if (facecache.positions.ptr)
		free(facecache.positions.ptr);
	pthread_mutex_destroy(&facecache.mtx);
}

static void face_cache_calculate(s32 size)
{
#define ADDPOS(a, b, c, va, vb, vc) \
	{ \
		v3s32 pos; \
		*(s32 *) ((char *) &pos + offsetof(v3s32, a)) = va; \
		*(s32 *) ((char *) &pos + offsetof(v3s32, b)) = vb; \
		*(s32 *) ((char *) &pos + offsetof(v3s32, c)) = vc; \
		array_append(&facecache.positions, &pos); \
	}
#define SQUARES(a, b, c) \
	for (s32 va = -size + 1; va < size; va++) { \
		for (s32 vb = -size + 1; vb < size; vb++) { \
			ADDPOS(a, b, c, va, vb,  size) \
			ADDPOS(a, b, c, va, vb, -size) \
		} \
	}
	SQUARES(x, z, y)
	SQUARES(x, y, z)
	SQUARES(z, y, x)
#undef SQUARES
#define EDGES(a, b, c) \
	for (s32 va = -size + 1; va < size; va++) { \
		ADDPOS(a, b, c, va,  size,  size) \
		ADDPOS(a, b, c, va,  size, -size) \
		ADDPOS(a, b, c, va, -size,  size) \
		ADDPOS(a, b, c, va, -size, -size) \
	}
	EDGES(x, y, z)
	EDGES(z, x, y)
	EDGES(y, x, z)
#undef EDGES
	ADDPOS(x, y, z,  size,  size,  size)
	ADDPOS(x, y, z,  size,  size, -size)
	ADDPOS(x, y, z,  size, -size,  size)
	ADDPOS(x, y, z,  size, -size, -size)
	ADDPOS(x, y, z, -size,  size,  size)
	ADDPOS(x, y, z, -size,  size, -size)
	ADDPOS(x, y, z, -size, -size,  size)
	ADDPOS(x, y, z, -size, -size, -size)
#undef ADDPOS
}

v3s32 facecache_face(size_t i, v3s32 *base)
{
	pthread_mutex_lock(&facecache.mtx);
	while (facecache.positions.siz <= i)
		face_cache_calculate(++facecache.size);
	v3s32 pos = ((v3s32 *) facecache.positions.ptr)[i];
	pthread_mutex_unlock(&facecache.mtx);
	if (base) {
		pos.x += base->x;
		pos.y += base->y;
		pos.z += base->z;
	}
	return pos;
}

size_t facecache_count(u32 size)
{
	size_t len = 1 + size * 2;
	return len * len * len;
}
