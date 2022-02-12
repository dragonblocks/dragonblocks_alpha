#ifndef _FRUSTUM_H_
#define _FRUSTUM_H_

#include <stdbool.h>
#include <linmath.h/linmath.h>
#include "types.h"

void frustum_update(mat4x4 view_proj);
bool frustum_is_visible(aabb3f32 box);

#endif
