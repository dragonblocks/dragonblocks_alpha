#ifndef _SERVER_PLAYER_H_
#define _SERVER_PLAYER_H_

#include <pthread.h>
#include <stdbool.h>
#include <dragonnet/peer.h>
#include "types.h"

typedef struct
{
	u64 id;                     // unique identifier
	DragonnetPeer *peer;
	pthread_rwlock_t ref;       // programming socks make you 100% cuter

	bool auth;
	char *name;                 // player name
	pthread_rwlock_t auth_lock; // why

	v3f64 pos;                  // player position
	pthread_rwlock_t pos_lock;  // i want to commit die
} ServerPlayer;

void server_player_init();
void server_player_deinit();

void server_player_add(DragonnetPeer *peer);
void server_player_remove(DragonnetPeer *peer);

u64 server_player_find(char *name);

ServerPlayer *server_player_grab(u64 id);
void server_player_drop(ServerPlayer *player);

bool server_player_auth(ServerPlayer *player, char *name);
void server_player_disconnect(ServerPlayer *player);
void server_player_send_pos(ServerPlayer *player);
void server_player_iterate(void (cb)(ServerPlayer *, void *), void *arg);

#endif
