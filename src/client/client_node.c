#include "biome.h"
#include "client/client.h"
#include "client/client_node.h"
#include "node.h"
#include "perlin.h"

static void render_state_biome(v3s32 pos, __attribute__((unused)) MapNode *node, Vertex3D *vertex)
{
	double min, max;
	min = 0.15;
	max = 0.45;

	vertex->color.h = get_wetness(pos) * (max - min) + min;
	vertex->color.s = 1.0f;
	vertex->color.v = 1.0f;
}

static void render_texture_offset(v3s32 pos, __attribute((unused)) MapNode *node, Vertex3D *vertex)
{
	vertex->textureCoordinates.s += smooth2d(((u32) 1 << 31) + pos.x, ((u32) 1 << 31) + pos.z, 0, seed + SO_TEXTURE_OFFSET_S);
	vertex->textureCoordinates.t += smooth2d(((u32) 1 << 31) + pos.x, ((u32) 1 << 31) + pos.z, 0, seed + SO_TEXTURE_OFFSET_T);
}

ClientNodeDefintion client_node_definitions[NODE_UNLOADED] = {
	// invalid
	{
		.texture_path = RESSOURCEPATH "textures/invalid.png",
		.texture = NULL,
		.render = NULL,
	},
	// air
	{
		.texture_path = NULL,
		.texture = NULL,
		.render = NULL,
	},
	// grass
	{
		.texture_path = RESSOURCEPATH "textures/grass.png",
		.texture = NULL,
		.render = &render_state_biome,
	},
	// dirt
	{
		.texture_path = RESSOURCEPATH "textures/dirt.png",
		.texture = NULL,
		.render = NULL,
	},
	// stone
	{
		.texture_path = RESSOURCEPATH "textures/stone.png",
		.texture = NULL,
		.render = &render_texture_offset,
	},
	// snow
	{
		.texture_path = RESSOURCEPATH "textures/snow.png",
		.texture = NULL,
		.render = NULL,
	},
};

void client_node_init()
{
	for (Node node = NODE_INVALID; node < NODE_UNLOADED; node++) {
		ClientNodeDefintion *def = &client_node_definitions[node];
		if (def->texture_path)
			def->texture = texture_get(def->texture_path);
	}
}
