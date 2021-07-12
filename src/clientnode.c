#include "client.h"
#include "clientnode.h"
#include "node.h"
#include "texture.h"

ClientNodeDefintion client_node_definitions[NODE_UNLOADED] = {
	{RESSOURCEPATH "textures/invalid.png", NULL},
	{NULL, NULL},
	{RESSOURCEPATH "textures/grass.png", NULL},
	{RESSOURCEPATH "textures/dirt.png", NULL},
	{RESSOURCEPATH "textures/stone.png", NULL},
};

void init_client_node_definitions()
{
	for (Node node = NODE_INVALID; node < NODE_UNLOADED; node++) {
		ClientNodeDefintion *def = &client_node_definitions[node];
		if (def->texture_path)
			def->texture = get_texture(def->texture_path);
	}
}
