#include <stdlib.h>
#include "array.h"
#include "facecache.h"

static struct
{
	Array positions;
	u32 size;
	pthread_mutex_t mtx;
} facecache;

__attribute((constructor)) static void init_face_cache()
{
	facecache.size = 0;
	facecache.positions = array_create(sizeof(v3s32));
	v3s32 pos = {0, 0, 0};
	array_append(&facecache.positions, &pos);
	pthread_mutex_init(&facecache.mtx, NULL);
}

__attribute((destructor)) void deinit_face_cache()
{
	if (facecache.positions.ptr)
		free(facecache.positions.ptr);
	pthread_mutex_destroy(&facecache.mtx);
}

static void calculate_face_cache(s32 size)
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

v3s32 get_face(size_t i, v3s32 *base)
{
	pthread_mutex_lock(&facecache.mtx);
	while (facecache.positions.siz <= i)
		calculate_face_cache(++facecache.size);
	v3s32 pos = ((v3s32 *) facecache.positions.ptr)[i];
	pthread_mutex_unlock(&facecache.mtx);
	if (base) {
		pos.x += base->x;
		pos.y += base->y;
		pos.z += base->z;
	}
	return pos;
}

size_t get_face_count(u32 size)
{
	size_t len = 1 + size * 2;
	return 1 + len * len * len;
}
