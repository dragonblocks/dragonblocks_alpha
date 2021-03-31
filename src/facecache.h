#ifndef _FACECACHE_H_
#define _FACECACHE_H_

#include <pthread.h>
#include "types.h"

v3s32 get_face(size_t i, v3s32 *base);
size_t get_face_count(u32 size);

#endif
