#ifndef _CLIENTPLAYER_H_
#define _CLIENTPLAYER_H_

#include "mesh.h"
#include "types.h"

typedef struct
{
	struct Client *client;
	v3f pos;
	v3f velocity;
	aabb3f box;
	f32 yaw, pitch;
	f32 eye_height;
	MeshObject *obj;
} ClientPlayer;

void clientplayer_init(struct Client *client);
void clientplayer_add_to_scene(ClientPlayer *player);
void clientplayer_jump(ClientPlayer *player);
void clientplayer_tick(ClientPlayer *player, f64 dtime);

#endif
