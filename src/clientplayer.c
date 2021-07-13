#include "camera.h"
#include "client.h"
#include "clientplayer.h"
#include "cube.h"
#include "texture.h"

static void update_pos(ClientPlayer *player)
{
	set_camera_position((v3f) {player->pos.x, player->pos.y + player->eye_height, player->pos.z});

	pthread_mutex_lock(&player->client->mtx);
	(void) (write_u32(player->client->fd, SC_POS) && write_v3f32(player->client->fd, player->pos));
	pthread_mutex_unlock(&player->client->mtx);

	player->obj->pos = player->pos;
	meshobject_transform(player->obj);
}

void clientplayer_init(Client *client)
{
	client->player.client = client;
	client->player.pos = (v3f) {0.0f, 200.0f, 0.0f};
	client->player.velocity = (v3f) {0.0f, 0.0f, 0.0f};
	client->player.box = (aabb3f) {{-0.3f, 0.0f, -0.3f}, {0.3f, 1.75f, 0.3f}};
	client->player.yaw = client->player.pitch = 0.0f;
	client->player.eye_height = 1.5f;
}

void clientplayer_add_to_scene(ClientPlayer *player)
{
	VertexBuffer buffer = vertexbuffer_create();
	vertexbuffer_set_texture(&buffer, get_texture(RESSOURCEPATH "textures/player.png"));

	for (int f = 0; f < 6; f++) {
		for (int v = 0; v < 6; v++) {
			Vertex vertex = cube_vertices[f][v];
			vertex.y += 0.5;
			vertexbuffer_add_vertex(&buffer, &vertex);
		}
	}

	player->obj = meshobject_create(buffer, player->client->scene, (v3f) {0.0f, 0.0f, 0.0f});
	player->obj->scale = (v3f) {0.6f, 1.75f, 0.6f};
	player->obj->visible = false;

	update_pos(player);
}

static aabb3f get_box(ClientPlayer *player)
{
	return (aabb3f) {
		{player->box.min.x + player->pos.x, player->box.min.y + player->pos.y, player->box.min.z + player->pos.z},
		{player->box.max.x + player->pos.x, player->box.max.y + player->pos.y, player->box.max.z + player->pos.z},
	};
}

static aabb3s32 round_box(aabb3f box)
{
	return (aabb3s32) {
		{floor(box.min.x + 0.5f), floor(box.min.y + 0.5f), floor(box.min.z + 0.5f)},
		{ceil(box.max.x - 0.5f), ceil(box.max.y - 0.5f), ceil(box.max.z - 0.5f)},
	};
}

static bool is_solid(Map *map, s32 x, s32 y, s32 z)
{
	Node node = map_get_node(map, (v3s32) {x, y, z}).type;
	return node == NODE_UNLOADED || node_definitions[node].solid;
}

static bool can_jump(ClientPlayer *player)
{
	aabb3f fbox = get_box(player);
	fbox.min.y -= 0.5f;

	aabb3s32 box = round_box(fbox);

	if (fbox.min.y - (f32) box.min.y > 0.01f)
		return false;

	for (s32 x = box.min.x; x <= box.max.x; x++)
		for (s32 z = box.min.z; z <= box.max.z; z++)
			if (is_solid(player->client->map, x, box.min.y, z))
				return true;

	return false;
}

void clientplayer_jump(ClientPlayer *player)
{
	if (can_jump(player))
		player->velocity.y += 10.0f;
}

void clientplayer_tick(ClientPlayer *player, f64 dtime)
{
	v3f old_pos = player->pos;
	v3f old_velocity = player->velocity;

	player->velocity.y -= 32.0f * dtime;

#define GETS(vec, comp) *(s32 *) ((char *) &vec + offsetof(v3s32, comp))
#define GETF(vec, comp) *(f32 *) ((char *) &vec + offsetof(v3f32, comp))
#define PHYSICS(a, b, c) { \
		f32 v = (GETF(player->velocity, a) + GETF(old_velocity, a)) / 2.0f; \
		if (v == 0.0f) \
			goto a ## _physics_done; \
		aabb3s32 box = round_box(get_box(player)); \
		v3f old_pos = player->pos; \
		GETF(player->pos, a) += v * dtime; \
		s32 dir; \
		f32 offset; \
		if (v > 0.0f) { \
			dir = +1; \
			offset = GETF(player->box.max, a); \
			GETS(box.min, a) = ceil(GETF(old_pos, a) + offset + 0.5f); \
			GETS(box.max, a) = floor(GETF(player->pos, a) + offset + 0.5f); \
		} else { \
			dir = -1; \
			offset = GETF(player->box.min, a); \
			GETS(box.min, a) = floor(GETF(old_pos, a) + offset - 0.5f); \
			GETS(box.max, a) = ceil(GETF(player->pos, a) + offset - 0.5f); \
		} \
		GETS(box.max, a) += dir; \
		for (s32 a = GETS(box.min, a); a != GETS(box.max, a); a += dir) { \
			for (s32 b = GETS(box.min, b); b <= GETS(box.max, b); b++) { \
				for (s32 c = GETS(box.min, c); c <= GETS(box.max, c); c++) { \
					if (is_solid(player->client->map, x, y, z)) { \
						GETF(player->pos, a) = (f32) a - offset - 0.5f * (f32) dir; \
						GETF(player->velocity, a) = 0.0f; \
						goto a ## _physics_done; \
					} \
				} \
			} \
		} \
		a ## _physics_done: (void) 0;\
	}

	PHYSICS(x, y, z)
	PHYSICS(y, x, z)
	PHYSICS(z, x, y)

#undef GETS
#undef GETF
#undef PHYSICS

	if (old_pos.x != player->pos.x || old_pos.y != player->pos.y || old_pos.z != player->pos.z)
		update_pos(player);
}
