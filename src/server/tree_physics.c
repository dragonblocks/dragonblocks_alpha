#include <assert.h>
#include <dragonstd/array.h>
#include <dragonstd/list.h>
#include <dragonstd/tree.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "facedir.h"
#include "server/server_node.h"
#include "server/server_terrain.h"
#include "server/tree_physics.h"
#include "server/voxel_depth_search.h"

typedef struct {
	v3s32 root;
	bool deadlock;
	Tree chunks;
} CheckTreeArg;

typedef struct {
	TerrainChunk *chunk;
	TerrainNode *node;
	u32 *tgs;
} CheckTreeSearchNodeMeta;

static inline bool is_tree_with_root(TerrainNode *node)
{
	switch (node->type) {
		NODES_TREE
			return ((TreeData *) node->data)->has_root;

		default:
			return false;
	}
}

static int cmp_chunk(const TerrainChunk *chunk, const v3s32 *pos)
{
	return v3s32_cmp(&chunk->pos, pos);
}

static void unlock_chunk(TerrainChunk *chunk)
{
	pthread_rwlock_unlock(&chunk->lock);
}

static void init_search_node(DepthSearchNode *search_node, CheckTreeArg *arg)
{
	// first, get chunk position and offset
	v3s32 chunkp = terrain_chunkp(search_node->pos);

	// check for chunk in cache
	TerrainChunk *chunk = tree_get(&arg->chunks, &chunkp, &cmp_chunk, NULL);

	// if not found in cache, get it from server_terrain and lock it
	if (!chunk) {
		chunk = terrain_get_chunk(server_terrain, chunkp, CHUNK_MODE_PASSIVE);

		// check if chunk is unloaded
		if (!chunk) {
			// if chunk is unloaded, don't remove the tree, it might have a connection to ground
			search_node->type = DEPTH_SEARCH_TARGET;

			// done
			return;
		}

		// try to obtain the chunk lock
		int lock_err = pthread_rwlock_wrlock(&chunk->lock);

		// a deadlock might occur because of the order the chunks are locked
		if (lock_err == EDEADLK) {
			// notify caller deadlock has occured
			arg->deadlock = true;

			// finish search directly
			search_node->type = DEPTH_SEARCH_TARGET;

			// done
			return;
		}

		// assert that no different error has occured while trying to obtain the lock
		assert(lock_err == 0);

		// insert chunk into cache
		tree_add(&arg->chunks, &chunk->pos, chunk, &cmp_chunk, NULL);
	}

	// get node offset
	v3s32 offset = terrain_offset(search_node->pos);

	// type coersion for easier access
	TerrainChunkMeta *meta = chunk->extra;

	// pointer to node and generation stage
	TerrainNode *node = &chunk->data[offset.x][offset.y][offset.z];
	u32 *tgs = &meta->tgsb.raw.nodes[offset.x][offset.y][offset.z];

	// type coersion for easier access
	TreeData *data = node->data;

	// have we found terrain?
	if (*tgs == STAGE_TERRAIN && node->type != NODE_AIR) {
		// if we've reached the target, set search node type accordingly
		search_node->type = DEPTH_SEARCH_TARGET;
	} else if (is_tree_with_root(node) && v3s32_equals(arg->root, data->root)) {
		// if node is part of our tree, continue search
		search_node->type = DEPTH_SEARCH_PATH;

		// allocate meta storage
		CheckTreeSearchNodeMeta *search_meta = search_node->extra = malloc(sizeof *search_meta);

		// store chunk, node and stage pointer for later
		search_meta->chunk = chunk;
		search_meta->node = node;
		search_meta->tgs = tgs;
	} else {
		// otherwise, this is a roadblock
		search_node->type = DEPTH_SEARCH_BLOCK;
	}
}

static void free_search_node(DepthSearchNode *node)
{
	if (node->extra)
		free(node->extra);

	free(node);
}

static void destroy_search_node(DepthSearchNode *node, List *changed_chunks)
{
	if (node->type == DEPTH_SEARCH_PATH && !(*node->success)) {
		// this is a tree/leaves node without connection to ground

		CheckTreeSearchNodeMeta *meta = node->extra;

		// overwrite node and generation stage
		*meta->node = server_node_create(NODE_AIR);
		*meta->tgs  = STAGE_PLAYER;

		// flag chunk as changed
		list_add(changed_chunks, meta->chunk, meta->chunk, &cmp_ref, NULL);
	}

	free_search_node(node);
}

/*
	Check whether all positions (that are part of the same tree) still are connected to the ground.
	Destroy any tree parts without ground connection.

	The advantage of grouping them together is that they can use the same search cache.
*/
static bool check_tree(v3s32 root, Array *positions, Array *chunks)
{
	CheckTreeArg arg;
	// inform depth search callbacks about root of tree (to only match nodes that belong to it)
	arg.root = root;
	// output parameter to prevent deadlocks
	arg.deadlock = false;
	// cache chunks, to accelerate lookup and prevent locking them twice
	tree_ini(&arg.chunks);

	// add the chunks the starting points are in to the chunk cache
	for (size_t i = 0; i < chunks->siz; i++) {
		TerrainChunk *chunk = ((TerrainChunk **) chunks->ptr)[i];
		tree_add(&arg.chunks, &chunk->pos, chunk, &cmp_chunk, NULL);
	}

	// nodes that have been visited
	// serves as search cache and contains all tree nodes, to remove them if no ground found
	Tree visit;
	tree_ini(&visit);

	// success means ground has been found

	// true if ground has been found for all positions
	bool success_all = true;
	// individual buffer for each start position (required by depth search algo)
	bool success_buf[positions->siz];

	// iterate over start positions
	for (size_t i = 0; i < positions->siz; i++) {
		success_buf[i] = false;

		// call depth search algorithm to collect positions and find ground
		if (!voxel_depth_search(((v3s32 *) positions->ptr)[i], (void *) &init_search_node, &arg,
				&success_buf[i], &visit))
			success_all = false;

		// immediately stop if there was a deadlock
		if (arg.deadlock)
			break;
	}

	if (success_all || arg.deadlock) {
		// ground has been found for all parts (or a deadlock was detected)

		// if ground has been found for all, there is no need to pass more complex callback
		tree_clr(&visit, &free_search_node, NULL, NULL, 0);

		// unlock grabbed chunks
		tree_clr(&arg.chunks, &unlock_chunk, NULL, NULL, 0);

		// return false if there was a deadlock - caller will reinitiate search
		return !arg.deadlock;
	}

	// keep track of changed chunks
	List changed_chunks;
	list_ini(&changed_chunks);

	// some or all positions have no connection to ground, pass callback to destroy nodes without
	tree_clr(&visit, &destroy_search_node, &changed_chunks, NULL, 0);

	// now, unlock all the chunks (before sending some of them)
	tree_clr(&arg.chunks, &unlock_chunk, NULL, NULL, 0);

	// send changed chunks
	server_terrain_lock_and_send_chunks(&changed_chunks);

	// done
	return true;
}

/*
	To be called after a node has been removed.
	For all neigbors that are part of a tree, check whether the tree now still has connection to
		the ground, destroy tree (partly) otherwise.

	- select neighbor nodes that are leaves or wood
	- in every iteration, select only the nodes that belong to the same tree (have the same root)
	- in every iteration, keep the chunks the selected nodes belong to locked
	- skip nodes that don't match the currently selected root, process them in a later iteration
*/
void tree_physics_check(v3s32 center)
{
	// remember directions that have been processed
	bool dirs[6] = {false};

	bool skipped;
	do {
		skipped = false;

		// the first node that has a root will initialize these variables
		bool selected_root = false;
		v3s32 root;

		// remember selected positions and their associated locked chunks
		Array positions, chunks;
		array_ini(&positions, sizeof(v3s32), 5);
		array_ini(&chunks, sizeof(TerrainChunk *), 5);

		// remember indices of positions selected in this iteration
		bool selected[6] = {false};

		for (int i = 0; i < 6; i++) {
			// we already processed this direction
			if (dirs[i])
				continue;

			// this may change back to false if we skip
			dirs[i] = true;

			// facedir contains offsets to neighbor nodes
			v3s32 pos = v3s32_add(center, facedir[i]);

			// get chunk
			v3s32 offset;
			TerrainChunk *chunk = terrain_get_chunk_nodep(server_terrain, pos, &offset, CHUNK_MODE_PASSIVE);
			if (!chunk)
				continue;

			// check if chunk is already locked
			bool locked_before = array_idx(&chunks, &chunk) != -1;

			// lock if not locked
			// fixme: a deadlock can happen here lol
			if (!locked_before)
				assert(pthread_rwlock_wrlock(&chunk->lock) == 0);

			// now that chunk is locked, actually get node
			TerrainNode *node = &chunk->data[offset.x][offset.y][offset.z];

			// check whether we're dealing with a tree node that has a root
			if (is_tree_with_root(node)) {
				// type coersion for easier access
				TreeData *data = node->data;

				// select root and initialize variables
				if (!selected_root) {
					selected_root = true;
					root = data->root;
				}

				// check whether root matches
				if (v3s32_equals(root, data->root)) {
					// remember position
					array_apd(&positions, &pos);

					// remember chunk - unless it's already on the list
					if (!locked_before)
						array_apd(&chunks, &chunk);

					// remember index was selected
					selected[i] = true;

					// don't run rest of loop body to not unlock chunk
					continue;
				} else {
					// if it doesn't match selected root, mark as skipped
					skipped = true;
					dirs[i] = false;
				}
			}

			// only unlock if it wasn't locked before
			if (!locked_before)
				pthread_rwlock_unlock(&chunk->lock);
		}

		if (selected_root) {
			// run depth search
			if (!check_tree(root, &positions, &chunks)) {
				// a return value of false means a deadlock occured (should be very rare)
				printf("[verbose] tree_physics detected deadlock (this not an issue, but should not happen frequently)\n");

				// sleep for 50ms to hopefully resolve the conflict
				nanosleep(&(struct timespec) {0, 50e6}, NULL);

				// invalidate faces that were selected in this iteration
				for (int i = 0; i < 6; i++)
					if (selected[i])
						dirs[i] = false;
			}

			// free memory
			array_clr(&positions);
			array_clr(&chunks);
		}

	// repeat until all directions have been processed
	} while (skipped);
}
