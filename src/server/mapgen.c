#include <stdio.h>
#include <math.h>
#include "environment.h"
#include "perlin.h"
#include "server/biomes.h"
#include "server/mapgen.h"
#include "server/server_map.h"
#include "util.h"

void mapgen_set_node(v3s32 pos, MapNode node, MapgenStage mgs, List *changed_blocks)
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

	v3s32 block_node_pos = {block->pos.x * MAPBLOCK_SIZE, block->pos.y * MAPBLOCK_SIZE, block->pos.z * MAPBLOCK_SIZE};
	v3s32 pos;

	for (u8 x = 0; x < MAPBLOCK_SIZE; x++) {
		pos.x = block_node_pos.x + x;

		for (u8 z = 0; z < MAPBLOCK_SIZE; z++) {
			pos.z = block_node_pos.z + z;

			s32 height = pnoise2d(U32(pos.x) / 32.0, U32(pos.z) / 32.0, 0.45, 5, seed + SO_HEIGHT) * 16.0 + 32;

			f64 biome_factor;
			BiomeDef *biome_def = get_biome(pos, &biome_factor);

			height = biome_def->height(pos, biome_factor, height);

			for (u8 y = 0; y < MAPBLOCK_SIZE; y++) {
				pos.y = block_node_pos.y + y;

				f64 wetness = get_wetness(pos);
				f64 temperature = get_temperature(pos);

				s32 diff = pos.y - height;

				Node node = biome_def->generate(pos, diff, wetness, temperature, biome_factor, block, changed_blocks);

				if (diff <= 1 && temperature < 0.0 && node == NODE_AIR)
					node = NODE_SNOW;

				pthread_mutex_lock(&block->mtx);
				if (extra->mgs_buffer[x][y][z] <= MGS_TERRAIN) {
					block->data[x][y][z] = map_node_create(node);
					extra->mgs_buffer[x][y][z] = MGS_TERRAIN;
				}
				pthread_mutex_unlock(&block->mtx);
			}
		}
	}
}
