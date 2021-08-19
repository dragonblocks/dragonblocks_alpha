#include <stdio.h>
#include <math.h>
#include "perlin.h"
#include "server/mapgen.h"
#include "server/server_map.h"

static void set_node(v3s32 pos, MapNode node, MapgenStage mgs, List *changed_blocks)
{
	MapgenSetNodeArg arg = {
		.mgs = mgs,
		.changed_blocks = changed_blocks,
	};
	map_set_node(server_map.map, pos, node, true, &arg);
}

// generate a block (does not manage block state or threading)
void mapgen_generate_block(MapBlock *block, List *changed_blocks)
{
	printf("Generating block at (%d, %d, %d)\n", block->pos.x, block->pos.y, block->pos.z);

	MapBlockExtraData *extra = block->extra;

	for (u8 x = 0; x < MAPBLOCK_SIZE; x++) {
		u32 ux = x + block->pos.x * MAPBLOCK_SIZE + ((u32) 1 << 31);
		for (u8 z = 0; z < MAPBLOCK_SIZE; z++) {
			u32 uz = z + block->pos.z * MAPBLOCK_SIZE + ((u32) 1 << 31);
			s32 height = smooth2d(ux / 32.0, uz / 32.0, 0, 0) * 16.0 + 128.0;
			bool is_mountain = false;

			double mountain_factor = (smooth2d(ux / 1000.0, uz / 1000.0, 0, 1) - 0.3) * 5.0;

			if (mountain_factor > 0.0) {
				height = pow(height * pow(((smooth2d(ux / 50.0, uz / 50.0, 2, 2) + 1.0) * 256.0 + 128.0), mountain_factor), 1.0 / (mountain_factor + 1.0));
				is_mountain = true;
			}

			for (u8 y = 0; y < MAPBLOCK_SIZE; y++) {
				s32 ay = block->pos.y * MAPBLOCK_SIZE + y;
				s32 diff = ay - height;

				Node node = NODE_AIR;

				if (diff < -5)
					node = NODE_STONE;
				else if (diff < -1)
					node = is_mountain ? NODE_STONE : NODE_DIRT;
				else if (diff < 0)
					node = is_mountain ? NODE_STONE : NODE_GRASS;
				else if (diff < 1)
					node = (is_mountain && ay > 256) ? NODE_SNOW : NODE_AIR;

				if (! is_mountain && diff == 0 && (smooth2d(x + block->pos.x * 16, z + block->pos.z * 16, 0, 3) * 0.5 + 0.5) < 0.0001f) {
					for (s8 bx = -1; bx <= 1; bx++) {
						for (s8 by = -1; by <= 1; by++) {
							for (s8 bz = -1; bz <= 1; bz++) {
								v3s32 pos = {block->pos.x * MAPBLOCK_SIZE + x + bx, block->pos.y * MAPBLOCK_SIZE + y + by, block->pos.z * MAPBLOCK_SIZE + z + bz};
								if (smooth3d(pos.x, pos.y, pos.z, 0, 4) > 0.0)
									set_node(pos, map_node_create(NODE_STONE), MGS_BOULDERS, changed_blocks);
							}
						}
					}
				}

				pthread_mutex_lock(&block->mtx);
				if (extra->mgs_buffer[x][y][z] <= MGS_TERRAIN) {
					block->data[x][y][z] = map_node_create(node);
					extra->mgs_buffer[x][y][z] = MGS_TERRAIN;

					if (node == NODE_GRASS) {
						double min, max;
						min = 0.15;
						max = 0.45;
						block->data[x][y][z].state.biome.x = (smooth2d(ux / 128.0, uz / 128.0, 0, 3) * 0.5 + 0.5) * (max - min) + min;
						block->data[x][y][z].state.biome.y = 1.0;
					}
				}
				pthread_mutex_unlock(&block->mtx);
			}
		}
	}
}
