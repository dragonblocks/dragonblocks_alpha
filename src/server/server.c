#define _GNU_SOURCE // don't worry, GNU extensions are only used when available
#include <dragonnet/addr.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "interrupt.h"
#include "server/database.h"
#include "server/server.h"
#include "server/server_player.h"
#include "server/server_terrain.h"

DragonnetListener *server;

static bool on_recv(DragonnetPeer *peer, DragonnetTypeId type, __attribute__((unused)) void *pkt)
{
	// this is recv thread, so we don't need lock_auth
	return ((ServerPlayer *) peer->extra)->auth != (type == DRAGONNET_TYPE_ToServerAuth);
}

static void on_ToServerAuth(DragonnetPeer *peer, ToServerAuth *pkt)
{
	if (server_player_auth(peer->extra, pkt->name))
		pkt->name = NULL;
}

// set a node on the map
static void on_ToServerSetnode(__attribute__((unused)) DragonnetPeer *peer, ToServerSetnode *pkt)
{
	terrain_set_node(server_terrain, pkt->pos,
		terrain_node_create(pkt->node, (Blob) {0, NULL}),
		false, NULL);
}

// update player's position
static void on_ToServerPosRot(DragonnetPeer *peer, ToServerPosRot *pkt)
{
	server_player_move(peer->extra, pkt->pos, pkt->rot);
}

// tell server map manager client requested the chunk
static void on_ToServerRequestChunk(DragonnetPeer *peer, ToServerRequestChunk *pkt)
{
	server_terrain_requested_chunk(peer->extra, pkt->pos);
}

// server entry point
int main(int argc, char **argv)
{
#ifdef __GLIBC__ // check whether bloat is enabled
	pthread_setname_np(pthread_self(), "main");
#endif // __GLIBC__

	if (argc < 2) {
		fprintf(stderr, "[error] missing address\n");
		return EXIT_FAILURE;
	}

	if (!(server = dragonnet_listener_new(argv[1]))) {
		fprintf(stderr, "[error] failed to listen to connections\n");
		return EXIT_FAILURE;
	}

	printf("[info] listening on %s\n", server->address);

	server->on_connect = &server_player_add;
	server->on_disconnect = &server_player_remove;
	server->on_recv = &on_recv;
	server->on_recv_type[DRAGONNET_TYPE_ToServerAuth        ] = (void *) &on_ToServerAuth;
	server->on_recv_type[DRAGONNET_TYPE_ToServerSetnode     ] = (void *) &on_ToServerSetnode;
	server->on_recv_type[DRAGONNET_TYPE_ToServerPosRot      ] = (void *) &on_ToServerPosRot;
	server->on_recv_type[DRAGONNET_TYPE_ToServerRequestChunk] = (void *) &on_ToServerRequestChunk;

	interrupt_init();
	if (!database_init())
		return EXIT_FAILURE;
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

	return EXIT_SUCCESS;
}
