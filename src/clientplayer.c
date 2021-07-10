#include "client.h"
#include "clientplayer.h"

void clientplayer_send_pos(ClientPlayer *player)
{
	pthread_mutex_lock(&player->client->mtx);
	(void) (write_u32(player->client->fd, SC_POS) && write_v3f32(player->client->fd, player->pos));
	pthread_mutex_unlock(&player->client->mtx);
}
