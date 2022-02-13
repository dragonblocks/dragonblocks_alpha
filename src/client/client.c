#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dragonstd/flag.h>
#include "client/client.h"
#include "client/client_auth.h"
#include "client/client_map.h"
#include "client/client_player.h"
#include "client/game.h"
#include "client/input.h"
#include "day.h"
#include "interrupt.h"
#include "perlin.h"
#include "types.h"
#include "util.h"

DragonnetPeer *client;
static Flag *finish;

static bool on_recv(unused DragonnetPeer *peer, DragonnetTypeId type, unused void *pkt)
{
	return (client_auth.state == AUTH_WAIT) == (type == DRAGONNET_TYPE_ToClientAuth);
}

static void on_disconnect(unused DragonnetPeer *peer)
{
	flag_set(interrupt);
	flag_wait(finish);
}

static void on_ToClientAuth(unused DragonnetPeer *peer, ToClientAuth *pkt)
{
	if (pkt->success) {
		client_auth.state = AUTH_SUCCESS;
		printf("Authenticated successfully\n");
	} else {
		client_auth.state = AUTH_INIT;
		printf("Authentication failed, please try again\n");
	}
}

static void on_ToClientBlock(unused DragonnetPeer *peer, ToClientBlock *pkt)
{
	MapBlock *block = map_get_block(client_map.map, pkt->pos, true);

	map_deserialize_block(block, pkt->data);
	((MapBlockExtraData *) block->extra)->all_air = (pkt->data.siz == 0);
	client_map_block_received(block);
}

static void on_ToClientInfo(unused DragonnetPeer *peer, ToClientInfo *pkt)
{
	client_map_set_simulation_distance(pkt->simulation_distance);
	seed = pkt->seed;
}

static void on_ToClientPos(unused DragonnetPeer *peer, ToClientPos *pkt)
{
	client_player_set_position(pkt->pos);
}

static void on_ToClientTimeOfDay(unused DragonnetPeer *peer, ToClientTimeOfDay *pkt)
{
	set_time_of_day(pkt->time_of_day);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Missing address\n");
		return EXIT_FAILURE;
	}

	if (! (client = dragonnet_connect(argv[1]))) {
		fprintf(stderr, "Failed to connect to server\n");
		return EXIT_FAILURE;
	}

	client->on_disconnect = &on_disconnect;
	client->on_recv = &on_recv;
	client->on_recv_type[DRAGONNET_TYPE_ToClientAuth     ] = (void *) &on_ToClientAuth;
	client->on_recv_type[DRAGONNET_TYPE_ToClientBlock    ] = (void *) &on_ToClientBlock;
	client->on_recv_type[DRAGONNET_TYPE_ToClientInfo     ] = (void *) &on_ToClientInfo;
	client->on_recv_type[DRAGONNET_TYPE_ToClientPos      ] = (void *) &on_ToClientPos;
	client->on_recv_type[DRAGONNET_TYPE_ToClientTimeOfDay] = (void *) &on_ToClientTimeOfDay;

	finish = flag_create();

	interrupt_init();
	client_map_init();
	client_player_init();
	dragonnet_peer_run(client);

	if (! client_auth_init())
		return EXIT_FAILURE;

	if (! game())
		return EXIT_FAILURE;

	dragonnet_peer_shutdown(client);
	client_auth_deinit();
	client_player_deinit();
	client_map_deinit();
	interrupt_deinit();

	pthread_t recv_thread = client->recv_thread;

	flag_set(finish);
	pthread_join(recv_thread, NULL);

	flag_delete(finish);

	return EXIT_SUCCESS;
}
