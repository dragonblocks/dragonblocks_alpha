#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "util.h"

static bool disconnect_handler(Client *client, bool good)
{
	if (good)
		server_disconnect_client(client, DISCO_NO_SEND, NULL);
	return true;
}

static bool auth_handler(Client *client, bool good)
{
	char *name = read_string(client->fd, NAME_MAX);

	if (! name)
		return false;

	if (! good) {
		free(name);
		return true;
	}

	u8 success = linked_list_put(&client->server->clients, name, client);

	printf("Authentication %s: %s -> %s\n", success ? "success" : "failure", client->address, name);

	if (success) {
		client->name = name;
		client->state = CS_ACTIVE;
	} else {
		free(name);
	}

	pthread_mutex_lock(&client->mtx);
	bool ret = write_u32(client->fd, CC_AUTH) && write_u8(client->fd, success);
	pthread_mutex_unlock(&client->mtx);

	return ret;
}

static bool getblock_handler(Client *client, bool good)
{
	v3s32 pos;

	if (! read_v3s32(client->fd, &pos))
		return false;

	if (! good)
		return true;

	MapBlock *block = map_get_block(client->server->map, pos, false);
	if (block) {
		pthread_mutex_lock(&client->mtx);
		bool ret = write_u32(client->fd, CC_BLOCK) && map_serialize_block(client->fd, block);
		pthread_mutex_unlock(&client->mtx);

		return ret;
	}

	return true;
}

static bool setnode_handler(Client *client, bool good)
{
	v3s32 pos;

	if (! read_v3s32(client->fd, &pos))
		return false;

	MapNode node;

	if (! map_deserialize_node(client->fd, &node))
		return false;

	if (good)
		map_set_node(client->server->map, pos, node);

	return true;
}

static bool kick_handler(Client *client, bool good)
{
	char *target_name = read_string(client->fd, NAME_MAX);

	if (! target_name)
		return false;

	if (good) {
		Client *target = linked_list_get(&client->server->clients, target_name);
		if (target)
			server_disconnect_client(target, 0, "kicked");
	}

	free(target_name);
	return true;
}

CommandHandler command_handlers[SERVER_COMMAND_COUNT] = {
	{0},
	{&disconnect_handler, "DISCONNECT", CS_CREATED | CS_ACTIVE},
	{&auth_handler, "AUTH", CS_CREATED},
	{&getblock_handler, "GETBLOCK", CS_ACTIVE},
	{&setnode_handler, "SETNODE", CS_ACTIVE},
	{&kick_handler, "KICK", CS_ACTIVE},
};
