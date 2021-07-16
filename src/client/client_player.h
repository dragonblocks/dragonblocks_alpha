#ifndef _CLIENT_PLAYER_H_
#define _CLIENT_PLAYER_H_

#include "client/client.h"
#include "client/hud.h"
#include "client/object.h"
#include "types.h"

extern struct ClientPlayer
{
	v3f pos;
	v3f velocity;
	aabb3f box;
	f32 yaw, pitch;
	f32 eye_height;
	Object *obj;
	Map *map;
	HUDElement *pos_display;
} client_player;

void client_player_init(Map *map);
void client_player_add_to_scene();
void client_player_jump();
void client_player_tick(f64 dtime);

#endif
