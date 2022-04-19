#ifndef _CLIENT_PLAYER_H_
#define _CLIENT_PLAYER_H_

#include <pthread.h>
#include "client/client_entity.h"
#include "types.h"

extern struct ClientPlayer {
	v3f64 velocity; // velocity is changed and read from the same thread, no lock needed
	ToClientMovement movement;
	pthread_rwlock_t lock_movement;
} client_player;

typedef struct {
	struct {
		ItemStack left;
		ItemStack right;
	} inventory;
	struct ClientPlayerBones {
		ModelNode *nametag;
		ModelNode *neck;
		ModelNode *eyes;
		ModelNode *arm_left;
		ModelNode *arm_right;
		ModelNode *hand_left;
		ModelNode *hand_right;
		ModelNode *leg_left;
		ModelNode *leg_right;
	} bones;
} ClientPlayerData;

void client_player_init();                           // called on startup
void client_player_deinit();                         // called on shutdown

void client_player_gfx_init();
void client_player_gfx_deinit();

ClientEntity *client_player_entity(u64 id);         // grab and return client entity by id
ClientEntity *client_player_entity_local();         // grab and return local client entity

void client_player_jump();                           // jump if possible

void client_player_update_pos(ClientEntity *entity); // entity needs to be the client entity
void client_player_update_rot(ClientEntity *entity); // entity needs to be the client entity

void client_player_tick(f64 dtime);                  // to be called every frame

#endif // _CLIENT_PLAYER_H_
