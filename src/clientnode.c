#include "client.h"
#include "clientnode.h"
#include "node.h"
#include "texture.h"

ClientNodeDefintion client_node_definitions[NODE_UNLOADED] = {
	{RESSOURCEPATH "textures/invalid.png", 0},
	{NULL, 0},
	{RESSOURCEPATH "textures/grass.png", 0},
	{RESSOURCEPATH "textures/dirt.png", 0},
	{RESSOURCEPATH "textures/stone.png", 0},
};

void init_client_node_definitions()
{
	for (Node node = NODE_INVALID; node < NODE_UNLOADED; node++) {
		ClientNodeDefintion *def = &client_node_definitions[node];
		if (def->texture_path)
			def->texture = get_texture(def->texture_path);
	}
}
