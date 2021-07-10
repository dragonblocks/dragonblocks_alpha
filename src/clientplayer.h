#ifndef _CLIENTPLAYER_H_
#define _CLIENTPLAYER_H_

#include "types.h"

typedef struct
{
	struct Client *client;
	v3f pos;
	f32 yaw, pitch;
} ClientPlayer;

void clientplayer_send_pos(ClientPlayer *player);

#endif
