#ifndef _FACECACHE_H_
#define _FACECACHE_H_

#include <pthread.h>
#include "types.h"

v3s32 facecache_get(size_t i);
size_t facecache_count(u32 size);

#endif // _FACECACHE_H_
