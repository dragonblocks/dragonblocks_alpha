#include <stdio.h>
#include <unistd.h>
#include "client/client.h"
#include "client/client_map.h"
#include "types.h"

static bool disconnect_handler(__attribute__((unused)) Client *client, bool good)
{
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
	v3s32 pos;

	if (! read_v3s32(client->fd, &pos))
		return false;

	size_t size;

	if (! read_u64(client->fd, &size))
		return false;

	if (size > sizeof(MapBlockData))	// guard to prevent malicious or malfunctioning packets from allocating huge unnecessary amounts of memory
		return false;

	char data[size];
	if (! read_full(client->fd, data, size))
		return false;

	MapBlock *block;

	if (good)
		block = map_get_block(client_map.map, pos, true);
	else
		block = map_allocate_block(pos);

	bool ret = map_deserialize_block(block, data, size);

	if (good)
		client_map_block_received(block);
	else
		map_free_block(block);

	return ret;
}

static bool simulation_distance_handler(Client *client, bool good)
{
	u32 simulation_distance;

	if (! read_u32(client->fd, &simulation_distance))
		return false;

	if (good)
		client_map_set_simulation_distance(simulation_distance);

	return true;
}

CommandHandler command_handlers[CLIENT_COMMAND_COUNT] = {
	{0},
	{&disconnect_handler, "DISCONNECT", CS_CREATED | CS_AUTH | CS_ACTIVE},
	{&auth_handler, "AUTH", CS_AUTH},
	{&block_handler, "BLOCK", CS_ACTIVE},
	{&simulation_distance_handler, "SIMULATION_DISTANCE", CS_ACTIVE},
};
