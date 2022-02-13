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

static s32 half_block_size = MAPBLOCK_SIZE / 2;

static void make_vertices(Object *object, MapBlock *block, bool hide_edges)
{
	object->visible = false;
	v3s32 node_bp = {
		block->pos.x * MAPBLOCK_SIZE,
		block->pos.y * MAPBLOCK_SIZE,
		block->pos.z * MAPBLOCK_SIZE
	};

	ITERATE_MAPBLOCK {
		MapNode *node = &block->data[x][y][z];
		ClientNodeDefinition *def = &client_node_definitions[node->type];

		if (def->visibility != NV_NONE) {
			v3f32 offset = {
				x - half_block_size - 0.5,
				y - half_block_size - 0.5,
				z - half_block_size - 0.5
			};

			for (int f = 0; f < 6; f++) {
				v3s8 npos = {
					x + fdir[f].x,
					y + fdir[f].y,
					z + fdir[f].z,
				};

				bool direct_neighbor = npos.x >= 0 && npos.x < MAPBLOCK_SIZE
					&& npos.y >= 0 && npos.y < MAPBLOCK_SIZE
					&& npos.z >= 0 && npos.z < MAPBLOCK_SIZE;

				MapNode neighbor = direct_neighbor
					? block->data[npos.x][npos.y][npos.z]
					: map_get_node(client_map.map, (v3s32) {npos.x + node_bp.x, npos.y + node_bp.y, npos.z + node_bp.z});

				bool transparency_edge = def->visibility != NV_BLEND || neighbor.type != node->type;

				bool render_node = def->visibility == NV_CLIP || (neighbor.type != NODE_UNLOADED
					&& client_node_definitions[neighbor.type].visibility != NV_SOLID
					&& transparency_edge);

				object->visible = object->visible || render_node;

				if (! hide_edges && ! direct_neighbor)
					render_node = transparency_edge;

				if (render_node) {
					object->transparent = object->transparent || def->visibility == NV_BLEND;
					object_set_texture(object, def->tiles.textures[f]);

					for (int v = 0; v < 6; v++) {
						Vertex3D vertex = cube_vertices[f][v];
						vertex.position.x += offset.x;
						vertex.position.y += offset.y;
						vertex.position.z += offset.z;

						if (def->render)
							def->render((v3s32) {x + node_bp.x, y + node_bp.y, z + node_bp.z}, node, &vertex, f, v);

						object_add_vertex(object, &vertex);
					}
				}
			}
		}
	}
}

static void animate_mapblock_mesh(Object *obj, f64 dtime)
{
	obj->scale.x += dtime * 2.0;

	if (obj->scale.x > 1.0f) {
		obj->scale.x = 1.0f;
		client_map_schedule_update_block_mesh(obj->extra);
	}

	obj->scale.z = obj->scale.y = obj->scale.x;

	object_transform(obj);
}

void blockmesh_make(MapBlock *block)
{
	MapBlockExtraData *extra = block->extra;

	Object *obj = object_create();

	obj->pos = (v3f32) {block->pos.x * MAPBLOCK_SIZE + half_block_size + 0.5f, block->pos.y * MAPBLOCK_SIZE + half_block_size + 0.5f, block->pos.z * MAPBLOCK_SIZE + half_block_size + 0.5f};
	obj->scale = (v3f32) {0.1f, 0.1f, 0.1f};
	obj->frustum_culling = true;
	obj->box = (aabb3f32) {{-half_block_size - 1.0f, -half_block_size - 1.0f, -half_block_size - 1.0f}, {half_block_size + 1.0f, half_block_size + 1.0f, half_block_size + 1.0f}};
	obj->on_render = (obj->scale.x == 1.0f) ? NULL : &animate_mapblock_mesh;
	obj->extra = block;

	make_vertices(obj, block, obj->scale.x == 1.0f);

	if (! object_add_to_scene(obj)) {
		object_delete(obj);
		obj = NULL;
	}

	pthread_mutex_lock(&block->mtx);
	if (extra->obj) {
		extra->obj->remove = true;

		obj->scale = extra->obj->scale;
		object_transform(obj);
	}

	extra->obj = obj;
	pthread_mutex_unlock(&block->mtx);
}
