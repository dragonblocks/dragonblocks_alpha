#include <stdlib.h>
#include "server/voxel_depth_search.h"

v3s32 dirs[6] = {
	{+0, -1, +0}, // this is commonly used to find ground, search downwards first
	{-1, +0, +0},
	{+0, +0, -1},
	{+1, +0, +0},
	{+0, +0, +1},
	{+0, +1, +0},
};

static int cmp_depth_search_node(const DepthSearchNode *node, const v3s32 *pos)
{
	return v3s32_cmp(&node->pos, pos);
}

bool voxel_depth_search(v3s32 pos, void (*callback)(DepthSearchNode *node, void *arg), void *arg, bool *success, Tree *visit)
{
	TreeNode **tree_node = tree_nfd(visit, &pos, &cmp_depth_search_node);
	if (*tree_node)
		return *success = *((DepthSearchNode *) (*tree_node)->dat)->success;

	DepthSearchNode *node = malloc(sizeof *node);
	tree_nmk(visit, tree_node, node);
	node->pos = pos;
	node->extra = NULL;
	callback(node, arg);
	if ((*(node->success = success) = (node->type == DEPTH_SEARCH_TARGET)))
		return true;

	if (node->type == DEPTH_SEARCH_PATH)
		for (int i = 0; i < 6; i++)
			if (voxel_depth_search(v3s32_add(pos, dirs[i]), callback, arg, success, visit))
				return true;

	return false;
}
