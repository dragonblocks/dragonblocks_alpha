#include <stdio.h>
#include <stdlib.h>
#include <dragonnet/addr.h>
#include "interrupt.h"
#include "server/database.h"
#include "server/server.h"
#include "server/server_map.h"
#include "server/server_player.h"
#include "util.h"

DragonnetListener *server;

static bool on_recv(DragonnetPeer *peer, DragonnetTypeId type, unused void *pkt)
{
	return ((ServerPlayer *) peer->extra)->auth != (type == DRAGONNET_TYPE_ToServerAuth);
}

static void on_ToServerAuth(DragonnetPeer *peer, ToServerAuth *pkt)
{
	if (server_player_auth(peer->extra, pkt->name))
		pkt->name = NULL;
}

// set a node on the map
static void on_ToServerSetnode(unused DragonnetPeer *peer, ToServerSetnode *pkt)
{
	map_set_node(server_map.map, pkt->pos, map_node_create(pkt->node, (Blob) {0, NULL}), false, NULL);
}

// update player's position
static void on_ToServerPos(DragonnetPeer *peer, ToServerPos *pkt)
{
	ServerPlayer *player = peer->extra;

	pthread_rwlock_wrlock(&player->pos_lock);
	player->pos = pkt->pos;
	database_update_player_pos(player->name, player->pos);
	pthread_rwlock_unlock(&player->pos_lock);
}

// tell server map manager client requested the block
static void on_ToServerRequestBlock(DragonnetPeer *peer, ToServerRequestBlock *pkt)
{
	server_map_requested_block(peer->extra, pkt->pos);
}

// server entry point
int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "Missing address\n");
		return EXIT_FAILURE;
	}

	if (! (server = dragonnet_listener_new(argv[1]))) {
		fprintf(stderr, "Failed to listen to connections\n");
		return EXIT_FAILURE;
	}

	char *address = dragonnet_addr_str(server->laddr);
	printf("Listening on %s\n", address);
	free(address);

	server->on_connect = &server_player_add;
	server->on_disconnect = &server_player_remove;
	server->on_recv = &on_recv;
	server->on_recv_type[DRAGONNET_TYPE_ToServerAuth] =         (void *) &on_ToServerAuth;
	server->on_recv_type[DRAGONNET_TYPE_ToServerSetnode] =      (void *) &on_ToServerSetnode;
	server->on_recv_type[DRAGONNET_TYPE_ToServerPos] =          (void *) &on_ToServerPos;
	server->on_recv_type[DRAGONNET_TYPE_ToServerRequestBlock] = (void *) &on_ToServerRequestBlock;

	interrupt_init();
	database_init();
	server_map_init();
	server_player_init();

	server_map_prepare_spawn();
	dragonnet_listener_run(server);

	flag_wait(interrupt);

	printf("Shutting down\n");
	dragonnet_listener_close(server);

	server_player_deinit();
	server_map_deinit();
	database_deinit();
	interrupt_deinit();

	dragonnet_listener_delete(server);

	return EXIT_SUCCESS;
}
