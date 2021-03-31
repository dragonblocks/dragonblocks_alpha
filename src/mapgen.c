#include "mapgen.h"
#include "perlin.h"

void mapgen_generate_block(MapBlock *block)
{
	for (u8 x = 0; x < 16; x++) {
		u32 ux = x + block->pos.x * 16 + ((u32) 1 << 31);
		for (u8 z = 0; z < 16; z++) {
			u32 uz = z + block->pos.z * 16 + ((u32) 1 << 31);
			s32 height = smooth2d((double) ux / 32.0f, (double) uz / 32.0f, 0, 0) * 16.0f;
			for (u8 y = 0; y < 16; y++) {
				s32 ay = y + block->pos.y * 16;
				Node type;
				if (ay > height)
					type = NODE_AIR;
				else if (ay == height)
					type = NODE_GRASS;
				else if (ay >= height - 4)
					type = NODE_DIRT;
				else
					type = NODE_STONE;
				block->data[x][y][z] = map_node_create(type);
				block->metadata[x][y][z] = list_create(&list_compare_string);
			}
		}
	}
}
