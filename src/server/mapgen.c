#include <stdio.h>
#include <math.h>
#include <stdlib.h>
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

	char *block_data[BIOME_COUNT] = {NULL};
	bool preprocessed_block[BIOME_COUNT];

	for (u8 x = 0; x < MAPBLOCK_SIZE; x++) {
		s32 pos_x = block_node_pos.x + x;

		for (u8 z = 0; z < MAPBLOCK_SIZE; z++) {
			v2s32 pos_horizontal = {pos_x, block_node_pos.z + z};

			s32 height = pnoise2d(U32(pos_horizontal.x) / 32.0, U32(pos_horizontal.y) / 32.0, 0.45, 5, seed + SO_HEIGHT) * 16.0 + 32;

			f64 factor;
			Biome biome = get_biome(pos_horizontal, &factor);
			BiomeDef *biome_def = &biomes[biome];

			if (biome_def->block_data_size > 0 && ! block_data[biome])
				block_data[biome] = malloc(biome_def->block_data_size);

			if (biome_def->preprocess_block && ! preprocessed_block[biome]) {
				biome_def->preprocess_block(block, changed_blocks, block_data[biome]);
				preprocessed_block[biome] = true;
			}

			char row_data[biome_def->row_data_size];

			if (biome_def->preprocess_row)
				biome_def->preprocess_row(pos_horizontal, height, factor, row_data, block_data[biome]);

			height = biome_def->height(pos_horizontal, factor, height, row_data, block_data[biome]);

			for (u8 y = 0; y < MAPBLOCK_SIZE; y++) {
				v3s32 pos = {pos_horizontal.x, block_node_pos.y + y, pos_horizontal.y};

				f64 humidity = get_humidity(pos);
				f64 temperature = get_temperature(pos);

				s32 diff = pos.y - height;

				Node node = biome_def->generate(pos, diff, humidity, temperature, factor, block, changed_blocks, row_data, block_data[biome]);

				if (biome_def->snow && diff <= 1 && temperature < 0.0 && node == NODE_AIR)
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

	for (Biome i = 0; i < BIOME_COUNT; i++) {
		if (block_data[i])
			free(block_data[i]);
	}
}
