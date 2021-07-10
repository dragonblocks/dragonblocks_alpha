#include "camera.h"
#include "client.h"
#include "clientplayer.h"

static void send_pos(ClientPlayer *player)
{
	pthread_mutex_lock(&player->client->mtx);
	(void) (write_u32(player->client->fd, SC_POS) && write_v3f32(player->client->fd, player->pos));
	pthread_mutex_unlock(&player->client->mtx);
}

void clientplayer_tick(ClientPlayer *player, f64 dtime)
{
	v3f old_pos = player->pos;

	player->pos.x += player->velocity.x * dtime;
	player->pos.y += player->velocity.y * dtime;
	player->pos.z += player->velocity.z * dtime;

	if (old_pos.x != player->pos.x || old_pos.y != player->pos.y || old_pos.z != player->pos.z) {
		send_pos(player);
		set_camera_position(player->pos);
	}
}
