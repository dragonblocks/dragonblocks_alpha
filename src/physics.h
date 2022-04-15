#ifndef _PHYSICS_H_
#define _PHYSICS_H_

#include <stdbool.h>
#include "terrain.h"
#include "types.h"

bool physics_ground(Terrain *terrain, bool collide, aabb3f32 box, v3f64 *pos, v3f64 *vel);
bool physics_step  (Terrain *terrain, bool collide, aabb3f32 box, v3f64 *pos, v3f64 *vel, v3f64 *acc, f64 t);

#endif // _PHYSICS_H_
