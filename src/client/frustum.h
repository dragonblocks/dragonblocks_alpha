#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include <stdbool.h>
#include <linmath.h>
#include "types.h"

extern mat4x4 frustum;

void frustum_update();
bool frustum_cull(aabb3f32 box);

#endif
