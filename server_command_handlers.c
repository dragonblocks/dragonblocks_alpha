#include <stdio.h>
#include "util.h"
#include "server.h"
#include "server_commands.h"
#include "server_command_handlers.h"

static bool disconnect_handler(Client *client)
{
	server_disconnect_client(client, 0, NULL);
	return true;
}

static bool auth_handler(Client *client)
{
	char *name = read_string(client->fd, NAME_MAX);

	if (! name)
		return false;

	if (linked_list_put(&client->server->clients, name, client)) {
		client->name = name;
		client->state = CS_ACTIVE;
		printf("Auth success: %s\n", server_get_client_name(client));
		if (! server_send_command(client, CC_AUTH_SUCCESS))
			return false;
	} else {
		printf("Auth failure: %s\n", server_get_client_name(client));
		if (! server_send_command(client, CC_AUTH_FAILURE))
			return false;
	}

	return true;
}

static bool getblock_handler(Client *client)
{
	v3s32 pos;

	if (! read_v3s32(client->fd, &pos))
		return false;

	MapBlock *block = map_get_block(client->server->map, pos, false);
	if (block) {
		pthread_mutex_lock(&client->mtx);
		bool ret = write_s32(client->fd, CC_BLOCK) && map_serialize_block(client->fd, block);
		pthread_mutex_unlock(&client->mtx);

		return ret;
	}

	return true;
}

static bool setnode_handler(Client *client)
{
	v3s32 pos;

	if (! read_v3s32(client->fd, &pos))
		return false;

	MapNode node;

	if (! map_deserialize_node(client->fd, &node))
		return false;

	map_set_node(client->server->map, pos, node);

	return true;
}

ServerCommandHandler server_command_handlers[SERVER_COMMAND_COUNT] = {
	{0},
	{&disconnect_handler, "DISCONNECT", CS_CREATED | CS_ACTIVE},
	{&auth_handler, "AUTH", CS_CREATED},
	{&getblock_handler, "GETBLOCK", CS_ACTIVE},
	{&setnode_handler, "SETNODE", CS_ACTIVE},
};
