#include <stdio.h>
#include "camera.h"
#include "client.h"
#include "clientplayer.h"

void clientplayer_send_pos(ClientPlayer *player)
{
	pthread_mutex_lock(&player->client->mtx);
	(void) (write_u32(player->client->fd, SC_POS) && write_v3f32(player->client->fd, player->pos));
	pthread_mutex_unlock(&player->client->mtx);
}

void clientplayer_init(Client *client)
{
	client->player.client = client;
	client->player.pos = (v3f) {0.0f, 25.0f, 0.0f};
	client->player.velocity = (v3f) {0.0f, 25.0f, 0.0f};
	client->player.box = (aabb3f) {{-0.3f, 0.0f, -0.3f}, {0.3f, 1.75f, 0.3f}};
	client->player.yaw = client->player.pitch = 0.0f;
	client->player.eye_height = 1.6f;
}

static bool collision(ClientPlayer *player)
{
	aabb3f box = {
		{player->box.min.x + player->pos.x, player->box.min.y + player->pos.y, player->box.min.z + player->pos.z},
		{player->box.max.x + player->pos.x, player->box.max.y + player->pos.y, player->box.max.z + player->pos.z},
	};

	for (s32 x = floor(box.min.x); x <= ceil(box.max.x - 0.01f) - 1; x++) {
		for (s32 y = floor(box.min.y); y <= ceil(box.max.y - 0.01f) - 1; y++) {
			for (s32 z = floor(box.min.z); z <= ceil(box.max.z - 0.01f) - 1; z++) {
				Node node = map_get_node(player->client->map, (v3s32) {x, y, z}).type;

				if (node == NODE_UNLOADED || node_definitions[node].solid)
					return true;
			}
		}
	}

	return false;
}

void clientplayer_tick(ClientPlayer *player, f64 dtime)
{
	v3f old_pos = player->pos;

#define CALC_PHYSICS(pos, velocity, old) \
	pos += velocity * dtime; \
	if (collision(player)) { \
		pos = old; \
		velocity = 0.0f; \
	}

	CALC_PHYSICS(player->pos.x, player->velocity.x, old_pos.x)
	CALC_PHYSICS(player->pos.y, player->velocity.y, old_pos.y)
	CALC_PHYSICS(player->pos.z, player->velocity.z, old_pos.z)

#undef CALC_PHYSICS

	// gravity
	player->velocity.y -= 9.81f * dtime;

	if (old_pos.x != player->pos.x || old_pos.y != player->pos.y || old_pos.z != player->pos.z) {
		clientplayer_send_pos(player);
		set_camera_position((v3f) {player->pos.x, player->pos.y + player->eye_height, player->pos.z});
	}
}
