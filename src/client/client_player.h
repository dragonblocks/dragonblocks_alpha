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

void client_player_init();                           // called on startup
void client_player_deinit();                         // called on shutdown

ClientEntity *client_player_entity();                // grab and return client entity

void client_player_jump();                           // jump if possible

void client_player_update_pos(ClientEntity *entity); // entity needs to be the client entity
void client_player_update_rot(ClientEntity *entity); // entity needs to be the client entity

void client_player_tick(f64 dtime);                  // to be called every frame

#endif // _CLIENT_PLAYER_H_
