#ifndef _CLIENTPLAYER_H_
#define _CLIENTPLAYER_H_

#include "types.h"

typedef struct
{
	struct Client *client;
	v3f pos;
	f32 yaw, pitch;
	v3f velocity;
} ClientPlayer;

void clientplayer_tick(ClientPlayer *player, f64 dtime);

#endif
