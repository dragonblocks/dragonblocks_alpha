#include "blockmesh.h"
#include "clientnode.h"

Vertex cube_vertices[6][6] = {
	{
		{-0.5, -0.5, -0.5, +0.0, +0.0},
		{+0.5, -0.5, -0.5, +1.0, +0.0},
		{+0.5, +0.5, -0.5, +1.0, +1.0},
		{+0.5, +0.5, -0.5, +1.0, +1.0},
		{-0.5, +0.5, -0.5, +0.0, +1.0},
		{-0.5, -0.5, -0.5, +0.0, +0.0},
	},
	{
		{-0.5, -0.5, +0.5, +0.0, +0.0},
		{+0.5, +0.5, +0.5, +1.0, +1.0},
		{+0.5, -0.5, +0.5, +1.0, +0.0},
		{+0.5, +0.5, +0.5, +1.0, +1.0},
		{-0.5, -0.5, +0.5, +0.0, +0.0},
		{-0.5, +0.5, +0.5, +0.0, +1.0},
	},
	{
		{-0.5, +0.5, +0.5, +1.0, +1.0},
		{-0.5, -0.5, -0.5, +0.0, +0.0},
		{-0.5, +0.5, -0.5, +0.0, +1.0},
		{-0.5, -0.5, -0.5, +0.0, +0.0},
		{-0.5, +0.5, +0.5, +1.0, +1.0},
		{-0.5, -0.5, +0.5, +1.0, +0.0},
	},
	{
		{+0.5, +0.5, +0.5, +1.0, +1.0},
		{+0.5, +0.5, -0.5, +0.0, +1.0},
		{+0.5, -0.5, -0.5, +0.0, +0.0},
		{+0.5, -0.5, -0.5, +0.0, +0.0},
		{+0.5, -0.5, +0.5, +1.0, +0.0},
		{+0.5, +0.5, +0.5, +1.0, +1.0},
	},
	{
		{-0.5, -0.5, -0.5, +0.0, +1.0},
		{+0.5, -0.5, -0.5, +1.0, +1.0},
		{+0.5, -0.5, +0.5, +1.0, +0.0},
		{+0.5, -0.5, +0.5, +1.0, +0.0},
		{-0.5, -0.5, +0.5, +0.0, +0.0},
		{-0.5, -0.5, -0.5, +0.0, +1.0},
	},
	{
		{-0.5, +0.5, -0.5, +0.0, +1.0},
		{+0.5, +0.5, -0.5, +1.0, +1.0},
		{+0.5, +0.5, +0.5, +1.0, +0.0},
		{+0.5, +0.5, +0.5, +1.0, +0.0},
		{-0.5, +0.5, +0.5, +0.0, +0.0},
		{-0.5, +0.5, -0.5, +0.0, +1.0},
	},
};

static v3s8 fdir[6] = {
	{+0, +0, -1},
	{+0, +0, +1},
	{-1, +0, +0},
	{+1, +0, +0},
	{+0, -1, +0},
	{+0, +1, +0},
};

#define VISIBLE(block, x, y, z) node_definitions[block->data[x][y][z].type].visible
#define VALIDPOS(pos) (pos.x >= 0 && pos.x < 16 && pos.y >= 0 && pos.y < 16 && pos.z >= 0 && pos.z < 16)

static VertexBuffer make_vertices(MapBlock *block)
{
	VertexBuffer buffer = vertexbuffer_create();

	ITERATE_MAPBLOCK {
		if (VISIBLE(block, x, y, z)) {
			v3f offset = {x + 8.5f, y + 8.5f, z + 8.5f};

			vertexbuffer_set_texture(&buffer, client_node_definitions[block->data[x][y][z].type].texture);

			for (int f = 0; f < 6; f++) {
				v3s8 npos = {
					x + fdir[f].x,
					y + fdir[f].y,
					z + fdir[f].z,
				};

				if (! VALIDPOS(npos) || ! VISIBLE(block, npos.x, npos.y, npos.z)) {
					for (int v = 0; v < 6; v++) {
						Vertex vertex = cube_vertices[f][v];
						vertex.x += offset.x;
						vertex.y += offset.y;
						vertex.z += offset.z;

						vertexbuffer_add_vertex(&buffer, &vertex);
					}
				}
			}
		}
	}

	return buffer;
}

#undef GNODDEF
#undef VALIDPOS

void make_block_mesh(MapBlock *block, Scene *scene)
{
	if (block->extra)
		((MeshObject *) block->extra)->remove = true;
	block->extra = meshobject_create(make_vertices(block), scene, (v3f) {block->pos.x * 16.0f - 8.0f, block->pos.y * 16.0f - 8.0f, block->pos.z * 16.0f - 8.0f});
}
