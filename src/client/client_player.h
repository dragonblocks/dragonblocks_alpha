#ifndef _CLIENT_PLAYER_H_
#define _CLIENT_PLAYER_H_

#include <pthread.h>
#include "client/client.h"
#include "client/object.h"
#include "types.h"

extern struct ClientPlayer
{
	v3f64 pos;               // feet position
	v3f64 velocity;          // current velocity
	aabb3f64 box;            // axis-aligned bounding box (used for collision), with 0, 0, 0 being the feet position
	f32 yaw, pitch;          // look direction
	f64 eye_height;          // eye height above feet
	pthread_rwlock_t rwlock; // used to protect the above properties
	bool fly;                // can the player fly?
	bool collision;          // should the player collide with the floor?
	Object *obj;             // 3D mesh object (currently always invisible), not thread safe
} client_player;

void client_player_init();                  // ClientPlayer singleton constructor
void client_player_deinit();                // ClientPlayer singleton destructor
void client_player_add_to_scene();          // create mesh object
void client_player_jump();                  // jump if possible
v3f64 client_player_get_position();         // get position (thread-safe)
void client_player_set_position(v3f64 pos); // set position (thread-safe)
void client_player_tick(f64 dtime);         // to be called every frame

#endif
