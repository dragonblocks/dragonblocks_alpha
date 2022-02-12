#include <stdio.h>
#include <stdlib.h>
#include "server/biomes.h"
#include "server/mapgen.h"
#include "server/trees.h"
#include "server/voxelctx.h"
#include "util.h"

// oak

static bool oak_condition(unused v3s32 pos, unused f64 humidity, unused f64 temperature, Biome biome, unused f64 factor, unused MapBlock *block, unused void *row_data, unused void *block_data)
{
	return biome == BIOME_HILLS;
}

static void oak_tree_leaf(Voxelctx *ctx)
{
	if (! voxelctx_is_alive(ctx))
		return;

	voxelctx_push(ctx);
		voxelctx_cube(ctx, NODE_OAK_LEAVES, true);
	voxelctx_pop(ctx);

	voxelctx_push(ctx);
		voxelctx_x(ctx, 0.5f);
		voxelctx_sx(ctx, 0.9f);
		voxelctx_sy(ctx, 0.9f);
		voxelctx_sz(ctx, 0.8f);
		voxelctx_ry(ctx, 25.0f);
		voxelctx_x(ctx, 0.4f);
		oak_tree_leaf(ctx);
	voxelctx_pop(ctx);
}

static void oak_tree_top(Voxelctx *ctx)
{
	if (! voxelctx_is_alive(ctx))
		return;

	voxelctx_push(ctx);
	for (int i = 0; i < 8; i++) {
		voxelctx_rz(ctx, 360.0f / 8.0f);
		voxelctx_push(ctx);
			voxelctx_life(ctx, 8);
			voxelctx_sy(ctx, 2.0f);
			voxelctx_z(ctx, voxelctx_random(ctx, 0.0f, 5.0f));
			voxelctx_s(ctx, 5.0f);
			voxelctx_light(ctx, -0.4f);
			voxelctx_sat(ctx, 0.5f);
			voxelctx_hue(ctx, voxelctx_random(ctx, 60.0f, 20.0f));
			voxelctx_ry(ctx, -45.0f);
			oak_tree_leaf(ctx);
		voxelctx_pop(ctx);
	}
	voxelctx_pop(ctx);
}

static void oak_tree_part(Voxelctx *ctx, f32 n)
{
	if (! voxelctx_is_alive(ctx))
		return;

	voxelctx_push(ctx);
	for (int i = 1; i <= n; i++) {
		voxelctx_z(ctx, 1.0f);
		voxelctx_rz(ctx, voxelctx_random(ctx, 30.0f, 10.0f));
		voxelctx_rx(ctx, voxelctx_random(ctx, 0.0f, 10.0f));

		voxelctx_push(ctx);
			voxelctx_s(ctx, 4.0f);
			voxelctx_x(ctx, 0.1f);
			voxelctx_light(ctx, voxelctx_random(ctx, 0.0f, 0.1f));
			voxelctx_cylinder(ctx, NODE_OAK_WOOD, true);
		voxelctx_pop(ctx);

		if (i == (int) (n - 2.0f)) {
			voxelctx_push(ctx);
				oak_tree_top(ctx);
			voxelctx_pop(ctx);
		}
	}
	voxelctx_pop(ctx);
}

static void oak_tree(v3s32 pos, List *changed_blocks)
{
	Voxelctx *ctx = voxelctx_create(changed_blocks, MGS_TREES, pos);

	voxelctx_hue(ctx, 40.0f);
	voxelctx_light(ctx, -0.5f);
	voxelctx_sat(ctx, 0.5f);

	f32 n = voxelctx_random(ctx, 40.0f, 10.0f);

	voxelctx_push(ctx);
	for (int i = 1; i <= 3; i++) {
		voxelctx_rz(ctx, voxelctx_random(ctx, 120.0f, 45.0f));
		voxelctx_push(ctx);
			voxelctx_y(ctx, 0.5f);
			voxelctx_light(ctx, voxelctx_random(ctx, -0.3f, 0.05f));
			oak_tree_part(ctx, n);
		voxelctx_pop(ctx);
	}
	voxelctx_pop(ctx);

	voxelctx_delete(ctx);
}

// pine

static bool pine_condition(unused v3s32 pos, unused f64 humidity, unused f64 temperature, Biome biome, unused f64 factor, unused MapBlock *block, unused void *row_data, unused void *block_data)
{
	return biome == BIOME_MOUNTAIN;
}

static void pine_tree(v3s32 pos, List *changed_blocks)
{
	s32 tree_top = (noise2d(pos.x, pos.z, 0, seed + SO_PINETREE_HEIGHT) * 0.5 + 0.5) * (35.0 - 20.0) + 20.0 + pos.y;
	for (v3s32 tree_pos = pos; tree_pos.y < tree_top; tree_pos.y++) {
		f64 branch_length = noise3d(tree_pos.x, tree_pos.y, tree_pos.z, 0, seed + SO_PINETREE_BRANCH) * 3.0;

		v3s32 dirs[4] = {
			{+0, +0, +1},
			{+1, +0, +0},
			{+0, +0, -1},
			{-1, +0, +0},
		};

		s32 dir = (noise3d(tree_pos.x, tree_pos.y, tree_pos.z, 0, seed + SO_PINETREE_BRANCH_DIR) * 0.5 + 0.5) * 4.0;

		for (v3s32 branch_pos = tree_pos; branch_length > 0; branch_length--, branch_pos = v3s32_add(branch_pos, dirs[dir]))
			mapgen_set_node(branch_pos, map_node_create(NODE_PINE_WOOD, (Blob) {0, NULL}), MGS_TREES, changed_blocks);

		mapgen_set_node(tree_pos, map_node_create(NODE_PINE_WOOD, (Blob) {0, NULL}), MGS_TREES, changed_blocks);
	}
}

// palm

static bool palm_condition(v3s32 pos, unused f64 humidity, unused f64 temperature, Biome biome, unused f64 factor, unused MapBlock *block, void *row_data, unused void *block_data)
{
	return biome == BIOME_OCEAN
		&& ocean_get_node_at((v3s32) {pos.x, pos.y - 0, pos.z}, 1, row_data) == NODE_AIR
		&& ocean_get_node_at((v3s32) {pos.x, pos.y - 1, pos.z}, 0, row_data) == NODE_SAND;
}

static void palm_branch(Voxelctx *ctx)
{
	if (! voxelctx_is_alive(ctx))
		return;

	voxelctx_cube(ctx, NODE_PALM_LEAVES, true);
	voxelctx_push(ctx);
		voxelctx_z(ctx, 0.5f);
		voxelctx_s(ctx, 0.8f);
		voxelctx_rx(ctx, voxelctx_random(ctx, 20.0f, 4.0f));
		voxelctx_z(ctx, 0.5f);
		palm_branch(ctx);
	voxelctx_pop(ctx);
}

static void palm_tree(v3s32 pos, List *changed_blocks)
{
	Voxelctx *ctx = voxelctx_create(changed_blocks, MGS_TREES, (v3s32) {pos.x, pos.y - 1, pos.z});

	f32 s = voxelctx_random(ctx, 8.0f, 2.0f);

	voxelctx_push(ctx);
	for (int i = 1; i <= s; i++) {
		voxelctx_z(ctx, 1.0f);
		voxelctx_push(ctx);
			voxelctx_s(ctx, 1.0f);
			voxelctx_light(ctx, voxelctx_random(ctx, -0.8f, 0.1f));
			voxelctx_sat(ctx, 0.5f);
			voxelctx_cube(ctx, NODE_PALM_WOOD, true);
		voxelctx_pop(ctx);
	}
	voxelctx_pop(ctx);

	voxelctx_z(ctx, s);
	voxelctx_sat(ctx, 1.0f),
	voxelctx_light(ctx, -0.5f);
	voxelctx_hue(ctx, voxelctx_random(ctx, 50.0f, 30.0f));

	voxelctx_push(ctx);
	for (int i = 0; i < 6; i++) {
		voxelctx_rz(ctx, 360.0f / 6.0f);
		voxelctx_rz(ctx, voxelctx_random(ctx, 0.0f, 10.0f));
		voxelctx_push(ctx);
			voxelctx_light(ctx, voxelctx_random(ctx, 0.0f, 0.3f));
			voxelctx_rx(ctx, 90.0f);
			voxelctx_s(ctx, 2.0f);
			palm_branch(ctx);
		voxelctx_pop(ctx);
	}
	voxelctx_pop(ctx);

	voxelctx_delete(ctx);
}

TreeDef tree_definitions[NUM_TREES] = {
	// oak
	{
		.spread = 64.0f,
		.probability = 0.0005f,
		.area_probability = 0.3f,
		.offset = SO_OAKTREE,
		.area_offset = SO_OAKTREE_AREA,
		.condition = &oak_condition,
		.generate = &oak_tree,
	},
	// pine
	{
		.spread = 256.0f,
		.probability = 0.01f,
		.area_probability = 0.1f,
		.offset = SO_PINETREE,
		.area_offset = SO_PINETREE_AREA,
		.condition = &pine_condition,
		.generate = &pine_tree,
	},
	// palm
	{
		.spread = 16.0f,
		.probability = 0.005f,
		.area_probability = 0.5f,
		.offset = SO_PALMTREE,
		.area_offset = SO_PALMTREE_AREA,
		.condition = &palm_condition,
		.generate = &palm_tree,
	},
};

