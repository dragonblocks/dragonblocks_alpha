#include <stdlib.h>
#include "server/biomes.h"
#include "server/server_terrain.h"
#include "server/trees.h"
#include "server/voxel_procedural.h"

// oak

static bool oak_condition(TreeArgsCondition *args)
{
	return args->biome == BIOME_HILLS;
}

static void oak_tree_leaf(VoxelProcedural *proc)
{
	if (!voxel_procedural_is_alive(proc))
		return;

	voxel_procedural_push(proc);
		voxel_procedural_cube(proc, NODE_OAK_LEAVES, true);
	voxel_procedural_pop(proc);

	voxel_procedural_push(proc);
		voxel_procedural_x(proc, 0.5f);
		voxel_procedural_sx(proc, 0.9f);
		voxel_procedural_sy(proc, 0.9f);
		voxel_procedural_sz(proc, 0.8f);
		voxel_procedural_ry(proc, 25.0f);
		voxel_procedural_x(proc, 0.4f);
		oak_tree_leaf(proc);
	voxel_procedural_pop(proc);
}

static void oak_tree_top(VoxelProcedural *proc)
{
	if (!voxel_procedural_is_alive(proc))
		return;

	voxel_procedural_push(proc);
	for (int i = 0; i < 8; i++) {
		voxel_procedural_rz(proc, 360.0f / 8.0f);
		voxel_procedural_push(proc);
			voxel_procedural_life(proc, 8);
			voxel_procedural_sy(proc, 2.0f);
			voxel_procedural_z(proc, voxel_procedural_random(proc, 0.0f, 5.0f));
			voxel_procedural_s(proc, 5.0f);
			voxel_procedural_light(proc, -0.4f);
			voxel_procedural_sat(proc, 0.5f);
			voxel_procedural_hue(proc, voxel_procedural_random(proc, 60.0f, 20.0f));
			voxel_procedural_ry(proc, -45.0f);
			oak_tree_leaf(proc);
		voxel_procedural_pop(proc);
	}
	voxel_procedural_pop(proc);
}

static void oak_tree_part(VoxelProcedural *proc, f32 n)
{
	if (!voxel_procedural_is_alive(proc))
		return;

	voxel_procedural_push(proc);
	for (int i = 1; i <= n; i++) {
		voxel_procedural_z(proc, 1.0f);
		voxel_procedural_rz(proc, voxel_procedural_random(proc, 30.0f, 10.0f));
		voxel_procedural_rx(proc, voxel_procedural_random(proc, 0.0f, 10.0f));

		voxel_procedural_push(proc);
			voxel_procedural_s(proc, 4.0f);
			voxel_procedural_x(proc, 0.1f);
			voxel_procedural_light(proc, voxel_procedural_random(proc, 0.0f, 0.1f));
			voxel_procedural_cylinder(proc, NODE_OAK_WOOD, true);
		voxel_procedural_pop(proc);

		if (i == (int) (n - 2.0f)) {
			voxel_procedural_push(proc);
				oak_tree_top(proc);
			voxel_procedural_pop(proc);
		}
	}
	voxel_procedural_pop(proc);
}

static void oak_tree(v3s32 pos, List *changed_chunks)
{
	VoxelProcedural *proc = voxel_procedural_create(changed_chunks, STAGE_TREES, pos);

	voxel_procedural_hue(proc, 40.0f);
	voxel_procedural_light(proc, -0.5f);
	voxel_procedural_sat(proc, 0.5f);

	f32 n = voxel_procedural_random(proc, 40.0f, 10.0f);

	voxel_procedural_push(proc);
	for (int i = 1; i <= 3; i++) {
		voxel_procedural_rz(proc, voxel_procedural_random(proc, 120.0f, 45.0f));
		voxel_procedural_push(proc);
			voxel_procedural_y(proc, 0.5f);
			voxel_procedural_light(proc, voxel_procedural_random(proc, -0.3f, 0.05f));
			oak_tree_part(proc, n);
		voxel_procedural_pop(proc);
	}
	voxel_procedural_pop(proc);

	voxel_procedural_delete(proc);
}

// pine

static bool pine_condition(TreeArgsCondition *args)
{
	return args->biome == BIOME_MOUNTAIN;
}

static void pine_tree(v3s32 pos, List *changed_chunks)
{
	s32 tree_top = (noise2d(pos.x, pos.z, 0, seed + OFFSET_PINETREE_HEIGHT) * 0.5 + 0.5) * (35.0 - 20.0) + 20.0 + pos.y;
	for (v3s32 tree_pos = pos; tree_pos.y < tree_top; tree_pos.y++) {
		f64 branch_length = noise3d(tree_pos.x, tree_pos.y, tree_pos.z, 0, seed + OFFSET_PINETREE_BRANCH) * 3.0;

		v3s32 dirs[4] = {
			{+0, +0, +1},
			{+1, +0, +0},
			{+0, +0, -1},
			{-1, +0, +0},
		};

		s32 dir = (noise3d(tree_pos.x, tree_pos.y, tree_pos.z, 0, seed + OFFSET_PINETREE_BRANCH_DIR) * 0.5 + 0.5) * 4.0;

		for (v3s32 branch_pos = tree_pos; branch_length > 0; branch_length--, branch_pos = v3s32_add(branch_pos, dirs[dir]))
			server_terrain_gen_node(branch_pos,
				terrain_node_create(NODE_PINE_WOOD, (Blob) {0, NULL}),
				STAGE_TREES, changed_chunks);

		server_terrain_gen_node(tree_pos,
			terrain_node_create(NODE_PINE_WOOD, (Blob) {0, NULL}),
			STAGE_TREES, changed_chunks);
	}
}

// palm

static bool palm_condition(TreeArgsCondition *args)
{
	return args->biome == BIOME_OCEAN
		&& ocean_get_node_at((v3s32) {args->pos.x, args->pos.y - 0, args->pos.z}, 1, args->row_data) == NODE_AIR
		&& ocean_get_node_at((v3s32) {args->pos.x, args->pos.y - 1, args->pos.z}, 0, args->row_data) == NODE_SAND;
}

static void palm_branch(VoxelProcedural *proc)
{
	if (!voxel_procedural_is_alive(proc))
		return;

	voxel_procedural_cube(proc, NODE_PALM_LEAVES, true);
	voxel_procedural_push(proc);
		voxel_procedural_z(proc, 0.5f);
		voxel_procedural_s(proc, 0.8f);
		voxel_procedural_rx(proc, voxel_procedural_random(proc, 20.0f, 4.0f));
		voxel_procedural_z(proc, 0.5f);
		palm_branch(proc);
	voxel_procedural_pop(proc);
}

static void palm_tree(v3s32 pos, List *changed_chunks)
{
	VoxelProcedural *proc = voxel_procedural_create(changed_chunks, STAGE_TREES, (v3s32) {pos.x, pos.y - 1, pos.z});

	f32 s = voxel_procedural_random(proc, 8.0f, 2.0f);

	voxel_procedural_push(proc);
	for (int i = 1; i <= s; i++) {
		voxel_procedural_z(proc, 1.0f);
		voxel_procedural_push(proc);
			voxel_procedural_s(proc, 1.0f);
			voxel_procedural_light(proc, voxel_procedural_random(proc, -0.8f, 0.1f));
			voxel_procedural_sat(proc, 0.5f);
			voxel_procedural_cube(proc, NODE_PALM_WOOD, true);
		voxel_procedural_pop(proc);
	}
	voxel_procedural_pop(proc);

	voxel_procedural_z(proc, s);
	voxel_procedural_sat(proc, 1.0f),
	voxel_procedural_light(proc, -0.5f);
	voxel_procedural_hue(proc, voxel_procedural_random(proc, 50.0f, 30.0f));

	voxel_procedural_push(proc);
	for (int i = 0; i < 6; i++) {
		voxel_procedural_rz(proc, 360.0f / 6.0f);
		voxel_procedural_rz(proc, voxel_procedural_random(proc, 0.0f, 10.0f));
		voxel_procedural_push(proc);
			voxel_procedural_light(proc, voxel_procedural_random(proc, 0.0f, 0.3f));
			voxel_procedural_rx(proc, 90.0f);
			voxel_procedural_s(proc, 2.0f);
			palm_branch(proc);
		voxel_procedural_pop(proc);
	}
	voxel_procedural_pop(proc);

	voxel_procedural_delete(proc);
}

TreeDef tree_def[NUM_TREES] = {
	// oak
	{
		.spread = 64.0f,
		.probability = 0.0005f,
		.area_probability = 0.3f,
		.offset = OFFSET_OAKTREE,
		.area_offset = OFFSET_OAKTREE_AREA,
		.condition = &oak_condition,
		.generate = &oak_tree,
	},
	// pine
	{
		.spread = 256.0f,
		.probability = 0.01f,
		.area_probability = 0.1f,
		.offset = OFFSET_PINETREE,
		.area_offset = OFFSET_PINETREE_AREA,
		.condition = &pine_condition,
		.generate = &pine_tree,
	},
	// palm
	{
		.spread = 16.0f,
		.probability = 0.005f,
		.area_probability = 0.5f,
		.offset = OFFSET_PALMTREE,
		.area_offset = OFFSET_PALMTREE_AREA,
		.condition = &palm_condition,
		.generate = &palm_tree,
	},
};

