#include <dragonnet/init.h>
#include <dragonstd/flag.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "client/client.h"
#include "client/client_auth.h"
#include "client/client_config.h"
#include "client/client_inventory.h"
#include "client/client_node.h"
#include "client/client_player.h"
#include "client/client_terrain.h"
#include "client/debug_menu.h"
#include "client/game.h"
#include "client/input.h"
#include "client/screenshot.h"
#include "common/day.h"
#include "common/init.h"
#include "common/interrupt.h"
#include "common/perlin.h"
#include "types.h"

DragonnetPeer *client;

static Flag finish;
static Flag gfx_init;

static bool on_recv(__attribute__((unused)) DragonnetPeer *peer, DragonnetTypeId type, __attribute__((unused)) void *pkt)
{
	bool allowed = false;
	pthread_mutex_lock(&client_auth.mtx);

	// this code exists to stop malicious or malfunctioning packets
	switch (client_auth.state) {
		// the server shouldn't send anything during auth preparation, drop it
		case AUTH_INIT:
			allowed = false;
			break;

		// only the auth packet is allowed before auth is finished
		case AUTH_WAIT:
			allowed = type == DRAGONNET_TYPE_ToClientAuth;
			break;

		// don't process auth packets when auth is already finished
		case AUTH_SUCCESS:
			allowed = type != DRAGONNET_TYPE_ToClientAuth;
			break;
	}

	/*
		It is important that the auth state does not change to until the packet is
			processed.

		However, the only state change done by other threads is AUTH_INIT -> AUTH_WAIT,
			which is not problematic since packets that are received during AUTH_INIT
			are not processed, they are always dropped.

		Therefore the mutex can be unlocked at this point.
	*/
	pthread_mutex_unlock(&client_auth.mtx);
	return allowed;
}

static void on_disconnect(__attribute__((unused)) DragonnetPeer *peer)
{
	flag_set(&interrupt);
	// don't free the connection before all other client components have shut down
	flag_slp(&finish);
}

static void on_ToClientAuth(__attribute__((unused)) DragonnetPeer *peer, ToClientAuth *pkt)
{
	pthread_mutex_lock(&client_auth.mtx);
	if (pkt->success) {
		client_auth.state = AUTH_SUCCESS;
		fprintf(stderr, "[access] authenticated successfully\n");
	} else {
		client_auth.state = AUTH_INIT;
	}
	pthread_cond_signal(&client_auth.cv);
	pthread_mutex_unlock(&client_auth.mtx);

	// yield the connection until the game is fully initialized
	if (pkt->success)
		flag_slp(&gfx_init);
}

static void on_ToClientInfo(__attribute__((unused)) DragonnetPeer *peer, ToClientInfo *pkt)
{
	client_terrain_set_load_distance(pkt->load_distance);
	seed = pkt->seed;
}

static void on_ToClientTimeOfDay(__attribute__((unused)) DragonnetPeer *peer, ToClientTimeOfDay *pkt)
{
	set_time_of_day(pkt->time_of_day);
}

static void on_ToClientMovement(__attribute__((unused)) DragonnetPeer *peer, ToClientMovement *pkt)
{
	pthread_rwlock_wrlock(&client_player.lock_movement);
	client_player.movement = *pkt;
	pthread_rwlock_unlock(&client_player.lock_movement);

	debug_menu_changed(ENTRY_FLIGHT);
	debug_menu_changed(ENTRY_COLLISION);
}

int main(int argc, char **argv)
{
	dragonblocks_init();

	char *config_path = "client.conf";
	bool exit_on_eof = false;

	struct option long_options[] = {
		{"config",         required_argument, 0, 'c' },
		{"exit-on-eof",    no_argument,       0, 'e' },
		{"screenshot-dir", required_argument, 0, 's' },
		{}
	};

	int option;
	while ((option = getopt_long(argc, argv, "c:es:", long_options, NULL)) != -1) {
		switch (option) {
			case 'c': config_path = optarg; break;
			case 'e': exit_on_eof = true; break;
			case 's': screenshot_dir = optarg; break;
		}
	}

	if (exit_on_eof) interrupt_exit_on_eof();

	client_config_load(config_path);

	if (argc-optind < 2) {
		fprintf(stderr, "[error] missing name or address\n");
		exit(EXIT_FAILURE);
	}

	if (!(client = dragonnet_connect(argv[optind+1]))) {
		fprintf(stderr, "[error] failed to connect to server\n");
		return EXIT_FAILURE;
	}

	fprintf(stderr, "[access] connected to %s\n", client->address);

	client->on_disconnect = &on_disconnect;
	client->on_recv                                                  = (void *) &on_recv;
	client->on_recv_type[DRAGONNET_TYPE_ToClientAuth               ] = (void *) &on_ToClientAuth;
	client->on_recv_type[DRAGONNET_TYPE_ToClientChunk              ] = (void *) &client_terrain_receive_chunk;
	client->on_recv_type[DRAGONNET_TYPE_ToClientInfo               ] = (void *) &on_ToClientInfo;
	client->on_recv_type[DRAGONNET_TYPE_ToClientTimeOfDay          ] = (void *) &on_ToClientTimeOfDay;
	client->on_recv_type[DRAGONNET_TYPE_ToClientMovement           ] = (void *) &on_ToClientMovement;
	client->on_recv_type[DRAGONNET_TYPE_ToClientEntityAdd          ] = (void *) &client_entity_add;
	client->on_recv_type[DRAGONNET_TYPE_ToClientEntityRemove       ] = (void *) &client_entity_remove;
	client->on_recv_type[DRAGONNET_TYPE_ToClientEntityUpdatePosRot ] = (void *) &client_entity_update_pos_rot;
	client->on_recv_type[DRAGONNET_TYPE_ToClientEntityUpdateNametag] = (void *) &client_entity_update_nametag;
	client->on_recv_type[DRAGONNET_TYPE_ToClientPlayerInventory    ] = (void *) &client_inventory_update_player;

	flag_ini(&finish);
	flag_ini(&gfx_init);

	interrupt_init();
	client_terrain_init();
	client_player_init();
	client_entity_init();
	client_auth_init();

	dragonnet_peer_run(client);
	client_auth_run(argv[optind]);

	game(&gfx_init);

	dragonnet_peer_shutdown(client);
	pthread_t recv_thread = client->recv_thread;

	flag_set(&finish);
	pthread_join(recv_thread, NULL);

	client_auth_deinit();
	client_entity_deinit();
	client_player_deinit();
	client_terrain_deinit();
	interrupt_deinit();

	flag_dst(&finish);
	flag_dst(&gfx_init);

	dragonnet_deinit();
	return EXIT_SUCCESS;
}
