#include <math.h>
#include "mapgen.h"
#include "perlin.h"

void mapgen_generate_block(MapBlock *block)
{
	for (u8 x = 0; x < 16; x++) {
		u32 ux = x + block->pos.x * 16 + ((u32) 1 << 31);
		for (u8 z = 0; z < 16; z++) {
			u32 uz = z + block->pos.z * 16 + ((u32) 1 << 31);
			s32 height = smooth2d(ux / 32.0, uz / 32.0, 0, 0) * 16.0 + 100.0;
			bool is_mountain = false;

			double mountain_factor = pow((smooth2d(ux / 1000.0, uz / 1000.0, 0, 1) - 0.3) * 5.0, 0.8);

			if (mountain_factor > 0.0) {
				height = pow(height * pow(((smooth2d(ux / 50.0, uz / 50.0, 0, 2) + 1.0) * 200.0 + 100.0), mountain_factor), 1.0 / (mountain_factor + 1.0));
				is_mountain = true;
			}

			for (u8 y = 0; y < 16; y++) {
				s32 ay = block->pos.y * 16 + y;
				s32 diff = ay - height;

				Node node = NODE_AIR;

				if (diff < -5)
					node = NODE_STONE;
				else if (diff < -1)
					node = is_mountain ? NODE_STONE : NODE_DIRT;
				else if (diff < 0)
					node = is_mountain ? NODE_STONE : NODE_GRASS;
				else if (diff < 1)
					node = (is_mountain && ay > 150) ? NODE_SNOW : NODE_AIR;

				block->data[x][y][z] = map_node_create(node);
				block->metadata[x][y][z] = list_create(&list_compare_string);
			}
		}
	}
}
