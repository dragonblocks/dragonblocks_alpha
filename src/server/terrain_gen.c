#include <math.h>
#include <stdlib.h>
#include "environment.h"
#include "perlin.h"
#include "server/biomes.h"
#include "server/server_terrain.h"
#include "server/terrain_gen.h"
#include "server/trees.h"

// generate a chunk (does not manage chunk state or threading)
void terrain_gen_chunk(TerrainChunk *chunk, List *changed_chunks)
{
	TerrainChunkMeta *meta = chunk->extra;

	BiomeArgsChunk chunk_args;
	BiomeArgsRow row_args;
	BiomeArgsHeight height_args;
	BiomeArgsGenerate generate_args;
	TreeArgsCondition condition_args;

	chunk_args.chunk = condition_args.chunk = chunk;
	chunk_args.changed_chunks = generate_args.changed_chunks = changed_chunks;

	v3s32 chunkp = {
		chunk->pos.x * CHUNK_SIZE,
		chunk->pos.y * CHUNK_SIZE,
		chunk->pos.z * CHUNK_SIZE,
	};

	unsigned char *chunk_data[COUNT_BIOME] = {NULL};
	bool chunk_called[COUNT_BIOME] = {false};

	for (u8 x = 0; x < CHUNK_SIZE; x++) {
		s32 pos_x = chunkp.x + x;

		for (u8 z = 0; z < CHUNK_SIZE; z++) {
			row_args.pos = height_args.pos = (v2s32) {pos_x, chunkp.z + z};

			condition_args.biome = get_biome(row_args.pos, &condition_args.factor);
			BiomeDef *biome_def = &biomes[condition_args.biome];

			height_args.factor = generate_args.factor = row_args.factor
				= condition_args.factor;

			if (biome_def->chunk_data_size && !chunk_data[condition_args.biome])
				chunk_data[condition_args.biome] =	malloc(biome_def->chunk_data_size);

			chunk_args.chunk_data = row_args.chunk_data = height_args.chunk_data =
				generate_args.chunk_data = condition_args.chunk_data =
				chunk_data[condition_args.biome];

			if (biome_def->chunk && !chunk_called[condition_args.biome]) {
				biome_def->chunk(&chunk_args);
				chunk_called[condition_args.biome] = true;
			}

			unsigned char row_data[biome_def->row_data_size];
			row_args.row_data = height_args.row_data = generate_args.row_data =
				condition_args.row_data = row_data;

			if (biome_def->row)
				biome_def->row(&row_args);

			height_args.height = 1.0
				* (pnoise2d(U32(height_args.pos.x) /  32.0, U32(height_args.pos.y) /  32.0, 0.45, 5, seed + SO_HEIGHT)    * 16.0 + 0.0)
				* (pnoise2d(U32(height_args.pos.x) / 256.0, U32(height_args.pos.y) / 256.0, 0.45, 5, seed + SO_HILLYNESS) *  0.5 + 0.5)
				+ 32.0;

			s32 height = biome_def->height(&height_args);

			for (u8 y = 0; y < CHUNK_SIZE; y++) {
				generate_args.pos = condition_args.pos = (v3s32)
					{row_args.pos.x, chunkp.y + y, row_args.pos.y};
				generate_args.diff = generate_args.pos.y - height;

				generate_args.humidity = condition_args.humidity =
					get_humidity(generate_args.pos);
				generate_args.temperature = condition_args.temperature =
					get_temperature(generate_args.pos);

				NodeType node = biome_def->generate(&generate_args);

				if (biome_def->snow
						&& generate_args.diff <= 1
						&& generate_args.temperature < 0.0
						&& node == NODE_AIR)
					node = NODE_SNOW;

				if (generate_args.diff == 1) for (int i = 0; i < NUM_TREES; i++) {
					TreeDef *def = &tree_definitions[i];

					if (def->condition(&condition_args)
							&& noise2d(condition_args.pos.x, condition_args.pos.z, 0, seed + def->offset) * 0.5 + 0.5 < def->probability
							&& smooth2d(U32(condition_args.pos.x) / def->spread, U32(condition_args.pos.z) / def->spread, 0, seed + def->area_offset) * 0.5 + 0.5 < def->area_probability) {
						def->generate(condition_args.pos, changed_chunks);
						break;
					}
				}

				pthread_mutex_lock(&chunk->mtx);
				if (meta->tgsb.raw.nodes[x][y][z] <= STAGE_TERRAIN) {
					chunk->data[x][y][z] = terrain_node_create(node, (Blob) {0, NULL});
					meta->tgsb.raw.nodes[x][y][z] = STAGE_TERRAIN;
				}
				pthread_mutex_unlock(&chunk->mtx);
			}
		}
	}

	for (Biome i = 0; i < COUNT_BIOME; i++)
		if (chunk_data[i])
			free(chunk_data[i]);
}
