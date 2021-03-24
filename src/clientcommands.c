#include <stdio.h>
#include "client.h"
#include "types.h"

static bool disconnect_handler(Client *client, bool good)
{
	if (good)
		client_disconnect(client, false, NULL);
	return true;
}

static bool auth_handler(Client *client, bool good)
{
	u8 success;
	if (! read_u8(client->fd, &success))
		return false;

	if (! good)
		return true;

	if (success) {
		printf("Authenticated successfully\n");
		client->state = CS_ACTIVE;
	} else {
		printf("Authentication failed, please try again\n");
		client->state = CS_CREATED;
	}

	return true;
}

static bool block_handler(Client *client, bool good)
{
	if (good)
		return map_deserialize_block(client->fd, client->map);
	return true;
}

CommandHandler command_handlers[CLIENT_COMMAND_COUNT] = {
	{0},
	{&disconnect_handler, "DISCONNECT", CS_CREATED | CS_AUTH | CS_ACTIVE},
	{&auth_handler, "AUTH", CS_AUTH},
	{&block_handler, "BLOCK", CS_ACTIVE},
};
