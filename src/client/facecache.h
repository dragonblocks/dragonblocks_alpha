#ifndef _FACECACHE_H_
#define _FACECACHE_H_

#include <pthread.h>
#include <dragontype/number.h>

v3s32 facecache_face(size_t i, v3s32 *base);
size_t facecache_count(u32 size);

#endif
