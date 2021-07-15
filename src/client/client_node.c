#include "client/client.h"
#include "client/client_node.h"
#include "node.h"

static void render_state_biome(MapNode *node, Vertex3D *vertex)
{
	vertex->color.h = node->state.biome.x;
	vertex->color.s = node->state.biome.y;
	vertex->color.v = node->state.biome.z;
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
		.render = NULL,
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
