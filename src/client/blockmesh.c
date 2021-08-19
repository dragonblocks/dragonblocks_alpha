#include "client/blockmesh.h"
#include "client/client_map.h"
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

static void make_vertices(Object *object, MapBlock *block)
{
	v3s32 node_bp = {block->pos.x * MAPBLOCK_SIZE, block->pos.y * MAPBLOCK_SIZE, block->pos.z * MAPBLOCK_SIZE};

	ITERATE_MAPBLOCK {
		MapNode *node = &block->data[x][y][z];

		if (node_definitions[node->type].visible) {
			v3f32 offset = {x + (f32) MAPBLOCK_SIZE / 2.0f, y + (f32) MAPBLOCK_SIZE / 2.0f, z + (f32) MAPBLOCK_SIZE / 2.0f};

			ClientNodeDefintion *client_node_def = &client_node_definitions[node->type];

			for (int f = 0; f < 6; f++) {
				v3s8 npos = {
					x + fdir[f].x,
					y + fdir[f].y,
					z + fdir[f].z,
				};

				Node neighbor;

				if (npos.x >= 0 && npos.x < MAPBLOCK_SIZE && npos.y >= 0 && npos.y < MAPBLOCK_SIZE && npos.z >= 0 && npos.z < MAPBLOCK_SIZE)
					neighbor = block->data[npos.x][npos.y][npos.z].type;
				else {
					MapNode nn = map_get_node(client_map.map, (v3s32) {npos.x + node_bp.x, npos.y + node_bp.y, npos.z + node_bp.z});
					neighbor = nn.type;
				}

				if (neighbor != NODE_UNLOADED && ! node_definitions[neighbor].visible) {
					object_set_texture(object, client_node_def->tiles.textures[f]);

					for (int v = 0; v < 6; v++) {
						Vertex3D vertex = cube_vertices[f][v];
						vertex.position.x += offset.x;
						vertex.position.y += offset.y;
						vertex.position.z += offset.z;

						if (client_node_def->render)
							client_node_def->render((v3s32) {x + node_bp.x, y + node_bp.y, z + node_bp.z}, node, &vertex, f, v);

						object_add_vertex(object, &vertex);
					}
				}
			}
		}
	}
}

void blockmesh_make(MapBlock *block)
{
	Object *obj = object_create();
	obj->pos = (v3f32) {block->pos.x * (f32) MAPBLOCK_SIZE - (f32) MAPBLOCK_SIZE / 2.0f, block->pos.y * (f32) MAPBLOCK_SIZE - (f32) MAPBLOCK_SIZE / 2.0f, block->pos.z * (f32) MAPBLOCK_SIZE - (f32) MAPBLOCK_SIZE / 2.0};

	make_vertices(obj, block);

	if (! object_add_to_scene(obj)) {
		object_delete(obj);
		obj = NULL;
	}

	MapBlockExtraData *extra = block->extra;

	if (extra->obj)
		extra->obj->remove = true;

	extra->obj = obj;
}
