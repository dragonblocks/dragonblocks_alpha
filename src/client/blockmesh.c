#include "client/blockmesh.h"
#include "client/client_node.h"
#include "client/cube.h"

static v3s8 fdir[6] = {
	{+0, +0, -1},
	{+0, +0, +1},
	{-1, +0, +0},
	{+1, +0, +0},
	{+0, -1, +0},
	{+0, +1, +0},
};

static void make_vertices(Object *object, MapBlock *block, Map *map)
{
	ITERATE_MAPBLOCK {
		MapNode *node = &block->data[x][y][z];

		if (node_definitions[node->type].visible) {
			v3f offset = {x + 8.0f, y + 8.0f, z + 8.0f};

			ClientNodeDefintion *client_def = &client_node_definitions[node->type];
			object_set_texture(object, client_def->texture);

			for (int f = 0; f < 6; f++) {
				v3s8 npos = {
					x + fdir[f].x,
					y + fdir[f].y,
					z + fdir[f].z,
				};

				Node neighbor;

				if (npos.x >= 0 && npos.x < 16 && npos.y >= 0 && npos.y < 16 && npos.z >= 0 && npos.z < 16)
					neighbor = block->data[npos.x][npos.y][npos.z].type;
				else
					neighbor = map_get_node(map, (v3s32) {npos.x + block->pos.x * 16, npos.y + block->pos.y * 16, npos.z + block->pos.z * 16}).type;

				if (neighbor != NODE_UNLOADED && ! node_definitions[neighbor].visible) {
					for (int v = 0; v < 6; v++) {
						Vertex3D vertex = cube_vertices[f][v];
						vertex.position.x += offset.x;
						vertex.position.y += offset.y;
						vertex.position.z += offset.z;

						if (client_def->render)
							client_def->render(node, &vertex);

						object_add_vertex(object, &vertex);
					}
				}
			}
		}
	}
}

void blockmesh_make(MapBlock *block, Map *map)
{
	Object *obj = object_create();
	obj->pos = (v3f) {block->pos.x * 16.0f - 8.0f, block->pos.y * 16.0f - 8.0f, block->pos.z * 16.0f - 8.0f};

	make_vertices(obj, block, map);

	if (! object_add_to_scene(obj)) {
		object_delete(obj);
		obj = NULL;
	}

	if (block->extra)
		((Object *) block->extra)->remove = true;

	block->extra = obj;
}
