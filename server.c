#include <assert.h>
#include "map.h"

int main()
{
	Map *map = map_create(NULL);
	map_set_node(map, (v3s32) {0, 0, 0}, map_node_create(NODE_STONE));
	printf("test 1 passed\n");
	map_set_node(map, (v3s32) {0, 5, 89}, map_node_create(NODE_DIRT));
	printf("test 2 passed\n");
	map_set_node(map, (v3s32) {321, 0, 89}, map_node_create(NODE_GRASS));
	printf("test 3 passed\n");
	map_set_node(map, (v3s32) {3124, 99, 2}, map_node_create(NODE_GRASS));
	printf("test 4 passed\n");
	MapBlock *block = map_get_block(map, (v3s32) {0, 0, 0}, false);
	assert(block);
	printf("(0 | 0 | 0) Block dump:\n");
	ITERATE_MAPBLOCK printf("%d", block->data[x][y][z].type);

	map_delete(map);
}
