#include <stdio.h>
#include "client.h"
#include "clientmap.h"
#include "types.h"

static bool disconnect_handler(Client *client, bool good)
{
	(void) client;

	if (good)
		client_disconnect(false, NULL);
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
	MapBlock *block;
	if (! map_deserialize_block(client->fd, client->map, &block, ! good))
		return false;
	if (good)
		clientmap_block_changed(block);
	return true;
}

CommandHandler command_handlers[CLIENT_COMMAND_COUNT] = {
	{0},
	{&disconnect_handler, "DISCONNECT", CS_CREATED | CS_AUTH | CS_ACTIVE},
	{&auth_handler, "AUTH", CS_AUTH},
	{&block_handler, "BLOCK", CS_ACTIVE},
};
