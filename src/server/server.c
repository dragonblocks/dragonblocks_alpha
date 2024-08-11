#include <dragonnet/addr.h>
#include <dragonnet/init.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common/init.h"
#include "common/interrupt.h"
#include "server/database.h"
#include "server/server.h"
#include "server/server_config.h"
#include "server/server_item.h"
#include "server/server_player.h"
#include "server/server_terrain.h"

DragonnetListener *server;

static bool on_recv(DragonnetPeer *peer, DragonnetTypeId type, __attribute__((unused)) void *pkt)
{
	// this is recv thread, so we don't need lock_auth
	return ((ServerPlayer *) peer->user)->auth != (type == DRAGONNET_TYPE_ToServerAuth);
}

static void on_ToServerAuth(DragonnetPeer *peer, ToServerAuth *pkt)
{
	if (server_player_auth(peer->user, pkt->name))
		pkt->name = NULL;
}

static void on_ToServerInteract(DragonnetPeer *peer, ToServerInteract *pkt)
{
	ServerPlayer *player = peer->user;
	pthread_mutex_lock(&player->mtx_inv);

	ItemStack *stack = &player->inventory.hands[pkt->right ? 1 : 0];
	if (server_item_def[stack->type].use)
		server_item_def[stack->type].use(player, stack, pkt->pointed, pkt->pos);

	pthread_mutex_unlock(&player->mtx_inv);
}

// update player's position
static void on_ToServerPosRot(DragonnetPeer *peer, ToServerPosRot *pkt)
{
	server_player_move(peer->user, pkt->pos, pkt->rot);
}

// tell server map manager client requested the chunk
static void on_ToServerRequestChunk(DragonnetPeer *peer, ToServerRequestChunk *pkt)
{
	server_terrain_requested_chunk(peer->user, pkt->pos);
}

static void on_ToServerInventorySwap(DragonnetPeer *peer, ToServerInventorySwap *pkt)
{
	server_player_inventory_swap(peer->user, pkt);
}

// server entry point
int main(int argc, char **argv)
{
	dragonblocks_init();

	char *config_path = "server.conf";
	char *world_path = ".";
	char *write_address = NULL;

	struct option long_options[] = {
		{"config",        required_argument, 0, 'c' },
		{"world",         required_argument, 0, 'w' },
		{"write-address", required_argument, 0, 'a' },
		{}
	};

	int option;
	while ((option = getopt_long(argc, argv, "c:w:a:", long_options, NULL)) != -1) {
		switch (option) {
			case 'c': config_path = optarg; break;
			case 'w': world_path = optarg; break;
			case 'a': write_address = optarg; break;
		}
	}

	server_config_load(config_path);

	if (argc-optind < 1) {
		fprintf(stderr, "[error] missing address\n");
		exit(EXIT_FAILURE);
	}

	if (!(server = dragonnet_listener_new(argv[optind]))) {
		fprintf(stderr, "[error] failed to listen to connections\n");
		return EXIT_FAILURE;
	}

	if (write_address) {
		FILE *addrfile = fopen(write_address, "a");
		if (!addrfile) {
			fprintf(stderr, "[error] failed to open address file\n");
			return EXIT_FAILURE;
		}

		fprintf(addrfile, "%s\n", server->address);
		fclose(addrfile);
	}

	printf("[info] listening on %s\n", server->address);

	server->on_connect = &server_player_add;
	server->on_disconnect = &server_player_remove;
	server->on_recv = &on_recv;
	server->on_recv_type[DRAGONNET_TYPE_ToServerAuth         ] = (void *) &on_ToServerAuth;
	server->on_recv_type[DRAGONNET_TYPE_ToServerInteract     ] = (void *) &on_ToServerInteract;
	server->on_recv_type[DRAGONNET_TYPE_ToServerPosRot       ] = (void *) &on_ToServerPosRot;
	server->on_recv_type[DRAGONNET_TYPE_ToServerRequestChunk ] = (void *) &on_ToServerRequestChunk;
	server->on_recv_type[DRAGONNET_TYPE_ToServerInventorySwap] = (void *) &on_ToServerInventorySwap;

	srand(time(0));

	interrupt_init();
	database_init(world_path);
	server_terrain_init();
	server_player_init();

	server_terrain_prepare_spawn();
	dragonnet_listener_run(server);

	flag_slp(&interrupt);

	printf("[info] shutting down\n");
	dragonnet_listener_close(server);

	server_player_deinit();
	server_terrain_deinit();
	database_deinit();
	interrupt_deinit();

	dragonnet_listener_delete(server);
	dragonnet_deinit();
	return EXIT_SUCCESS;
}
