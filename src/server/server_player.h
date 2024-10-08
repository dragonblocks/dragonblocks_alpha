#ifndef _SERVER_PLAYER_H_
#define _SERVER_PLAYER_H_

#include <dragonnet/peer.h>
#include <dragonstd/refcount.h>
#include <pthread.h>
#include <stdbool.h>
#include "common/item.h"
#include "types.h"

typedef struct {
	u64 id;                        // unique identifier
	Refcount rc;                   // delete yourself if no one cares about you

	DragonnetPeer *peer;           // not to be confused with beer
	pthread_rwlock_t lock_peer;    // programming socks make you 100% cuter

	bool auth;                     // YES OR NO I DEMAND AN ANSWER
	char *name;                    // player name
	pthread_rwlock_t lock_auth;    // poggers based cringe

	v3f64 pos;                     // player position
	v3f32 rot;                     // you wont guess what this is
	pthread_rwlock_t lock_pos;     // git commit crime

	struct {
		ItemStack hands[INV_SIZE_HANDS];
		ItemStack main[INV_SIZE_MAIN];
	} inventory;
	pthread_mutex_t mtx_inv;
} ServerPlayer;

void server_player_init();
void server_player_deinit();

void server_player_add(DragonnetPeer *peer);
void server_player_remove(DragonnetPeer *peer);

ServerPlayer *server_player_grab(u64 id);
ServerPlayer *server_player_grab_named(char *name);

bool server_player_auth(ServerPlayer *player, char *name);
void server_player_disconnect(ServerPlayer *player);
void server_player_move(ServerPlayer *player, v3f64 pos, v3f32 rot);
void server_player_iterate(void *func, void *arg);
void server_player_inventory_changed(ServerPlayer *player);
void server_player_inventory_swap(ServerPlayer *player, ToServerInventorySwap *pkt);

#endif // _SERVER_PLAYER_H_
