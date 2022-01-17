#include <stdio.h>
#include <stdlib.h>
#include "day.h"
#include "server/database.h"
#include "server/server.h"
#include "server/server_map.h"
#include "perlin.h"
#include "util.h"

// command callbacks

// disconnect client without sending a packet (client won't recieve it)
static bool disconnect_handler(Client *client, bool good)
{
	if (good)
		server_disconnect_client(client, DISCO_NO_SEND, NULL);
	return true;
}

// insert client into player list and update state (if checks pass)
static bool auth_handler(Client *client, bool good)
{
	char *name = read_string(client->fd, PLAYER_NAME_MAX);

	if (! name)
		return false;

	if (! good) {
		free(name);
		return true;
	}

	pthread_rwlock_wrlock(&client->server->players_rwlck);
	u8 success = list_put(&client->server->players, name, client);

	if (success) {
		client->name = name;
		client->state = CS_ACTIVE;

		if (! database_load_player(client->name, &client->pos)) {
			client->pos = (v3f64) {0.0, server_map.spawn_height + 0.5, 0.0};

			database_create_player(client->name, client->pos);
		}
	} else {
		free(name);
	}

	pthread_mutex_lock(&client->mtx);
	bool ret = write_u32(client->fd, CC_AUTH) && write_u8(client->fd, success);
	if (ret && success)
		ret = ret
			&& write_u32(client->fd, CC_INFO) && write_u32(client->fd, client->server->config.simulation_distance) && write_s32(client->fd, seed)
			&& write_u32(client->fd, CC_SETPOS) && write_v3f64(client->fd, client->pos)
			&& write_u32(client->fd, CC_TIMEOFDAY) && write_u64(client->fd, (u64) get_time_of_day());
	pthread_mutex_unlock(&client->mtx);

	pthread_rwlock_unlock(&client->server->players_rwlck);

	printf("Authentication %s: %s -> %s\n", success ? "success" : "failure", client->address, name);

	return ret;
}

// set a node on the map
static bool setnode_handler(Client *client, bool good)
{
	v3s32 pos;

	if (! read_v3s32(client->fd, &pos))
		return false;

	Node node;

	if (! read_u32(client->fd, &node))
		return false;

	if (good)
		map_set_node(server_map.map, pos, map_node_create(node, NULL, 0), false, NULL);

	return true;
}

// update player's position
static bool pos_handler(Client *client, bool good)
{
	v3f64 pos;

	if (! read_v3f64(client->fd, &pos))
		return false;

	if (good) {
		client->pos = pos;
		database_update_player_pos(client->name, client->pos);
	}

	return true;
}

// tell server map manager client requested the block
static bool request_block_handler(Client *client, bool good)
{
	v3s32 pos;

	if (! read_v3s32(client->fd, &pos))
		return false;

	if (good)
		server_map_requested_block(client, pos);

	return true;
}

// declared in network.h
CommandHandler command_handlers[SERVER_COMMAND_COUNT] = {
	{0},
	{&disconnect_handler, "DISCONNECT", CS_CREATED | CS_ACTIVE},
	{&auth_handler, "AUTH", CS_CREATED},
	{&setnode_handler, "SETNODE", CS_ACTIVE},
	{&pos_handler, "POS", CS_ACTIVE},
	{&request_block_handler, "REQUEST_BLOCK", CS_ACTIVE},
};
