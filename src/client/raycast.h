#ifndef _RAYCAST_H_
#define _RAYCAST_H_

#include <stdbool.h>
#include "common/node.h"
#include "types.h"

bool raycast(v3f64 pos, v3f64 dir, f64 len, v3s32 *node_pos, NodeType *node);

#endif // _RAYCAST_H_
