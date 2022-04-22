#ifndef _VOXEL_DEPTH_SEARCH_
#define _VOXEL_DEPTH_SEARCH_

#include <dragonstd/tree.h>
#include <stdbool.h>
#include "types.h"

typedef enum {
	DEPTH_SEARCH_TARGET, // goal has been reached
	DEPTH_SEARCH_PATH,   // can used this as path
	DEPTH_SEARCH_BLOCK   // cannot use this as path
} DepthSearchNodeType;

typedef struct {
	v3s32 pos;
	DepthSearchNodeType type;
	bool *success;
	void *extra;
} DepthSearchNode;

bool voxel_depth_search(v3s32 pos, void (*callback)(DepthSearchNode *node, void *arg), void *arg, bool *success, Tree *visit);

#endif // _VOXEL_DEPTH_SEARCH_
