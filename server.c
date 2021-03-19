#include "map.h"

int main()
{
	Map *map = map_create(NULL);
	map_set_node(map, (v3s32) {0, 0, 0}, map_node_create(NODE_STONE));
	MapBlock *block = map_get_block(map, (v3s32) {0, 0, 0}, false);
	if (! block) {
		fprintf(stderr, "Map error\n");
		return 1;
	}
	ITERATE_MAPBLOCK printf("%d", block->data[x][y][z].type);
	map_delete(map);
}
