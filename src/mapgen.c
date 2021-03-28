#include <stdlib.h>
#include "mapgen.h"

// mapgen prototype
static void generate_block(MapBlock *block)
{
	ITERATE_MAPBLOCK {
		block->data[x][y][z] = map_node_create(rand() % 4 + 1);
	}
	block->ready = true;
}

void mapgen_init(Map *map)
{
	map->on_block_create = &generate_block;
}
