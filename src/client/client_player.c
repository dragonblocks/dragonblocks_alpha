#include <stdio.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_player.h"
#include "client/cube.h"
#include "client/texture.h"

struct ClientPlayer client_player;

static void update_pos()
{
	camera_set_position((v3f) {client_player.pos.x, client_player.pos.y + client_player.eye_height, client_player.pos.z});
	client_send_position(client_player.pos);

	client_player.obj->pos = client_player.pos;
	object_transform(client_player.obj);

	char pos_text[BUFSIZ];
	sprintf(pos_text, "(%.1f %.1f %.1f)", client_player.pos.x, client_player.pos.y, client_player.pos.z);

	hud_change_text(client_player.pos_display, pos_text);
}

void client_player_init(Map *map)
{
	client_player.map = map;
	client_player.pos = (v3f) {0.0f, 200.0f, 0.0f};
	client_player.velocity = (v3f) {0.0f, 0.0f, 0.0f};
	client_player.box = (aabb3f) {{-0.3f, 0.0f, -0.3f}, {0.3f, 1.75f, 0.3f}};
	client_player.yaw = client_player.pitch = 0.0f;
	client_player.eye_height = 1.5f;
}

void client_player_add_to_scene()
{
	client_player.obj = object_create();
	client_player.obj->scale = (v3f) {0.6f, 1.75f, 0.6f};
	client_player.obj->visible = false;

	object_set_texture(client_player.obj, texture_get(RESSOURCEPATH "textures/player.png"));

	for (int f = 0; f < 6; f++) {
		for (int v = 0; v < 6; v++) {
			Vertex3D vertex = cube_vertices[f][v];
			vertex.position.y += 0.5;
			object_add_vertex(client_player.obj, &vertex);
		}
	}

	client_player.pos_display = hud_add((HUDElementDefinition) {
		.type = HUD_TEXT,
		.pos = {-1.0f, -1.0f, 0.0f},
		.offset = {2, 2 + 16 + 2 + 16 + 2},
		.type_def = {
			.text = {
				.text = "",
				.color = {1.0f, 1.0f, 1.0f},
			},
		},
	});

	update_pos();
}

static aabb3f get_box()
{
	return (aabb3f) {
		{client_player.box.min.x + client_player.pos.x, client_player.box.min.y + client_player.pos.y, client_player.box.min.z + client_player.pos.z},
		{client_player.box.max.x + client_player.pos.x, client_player.box.max.y + client_player.pos.y, client_player.box.max.z + client_player.pos.z},
	};
}

static aabb3s32 round_box(aabb3f box)
{
	return (aabb3s32) {
		{floor(box.min.x + 0.5f), floor(box.min.y + 0.5f), floor(box.min.z + 0.5f)},
		{ceil(box.max.x - 0.5f), ceil(box.max.y - 0.5f), ceil(box.max.z - 0.5f)},
	};
}

static bool is_solid(s32 x, s32 y, s32 z)
{
	Node node = map_get_node(client_player.map, (v3s32) {x, y, z}).type;
	return node == NODE_UNLOADED || node_definitions[node].solid;
}

static bool can_jump()
{
	aabb3f fbox = get_box();
	fbox.min.y -= 0.5f;

	aabb3s32 box = round_box(fbox);

	if (fbox.min.y - (f32) box.min.y > 0.01f)
		return false;

	for (s32 x = box.min.x; x <= box.max.x; x++)
		for (s32 z = box.min.z; z <= box.max.z; z++)
			if (is_solid(x, box.min.y, z))
				return true;

	return false;
}

void client_player_jump()
{
	if (can_jump())
		client_player.velocity.y += 10.0f;
}

void client_player_tick(f64 dtime)
{
	v3f old_pos = client_player.pos;
	v3f old_velocity = client_player.velocity;

	client_player.velocity.y -= 32.0f * dtime;

#define GETS(vec, comp) *(s32 *) ((char *) &vec + offsetof(v3s32, comp))
#define GETF(vec, comp) *(f32 *) ((char *) &vec + offsetof(v3f32, comp))
#define PHYSICS(a, b, c) { \
		f32 v = (GETF(client_player.velocity, a) + GETF(old_velocity, a)) / 2.0f; \
		if (v == 0.0f) \
			goto a ## _physics_done; \
		aabb3s32 box = round_box(get_box()); \
		v3f old_pos = client_player.pos; \
		GETF(client_player.pos, a) += v * dtime; \
		s32 dir; \
		f32 offset; \
		if (v > 0.0f) { \
			dir = +1; \
			offset = GETF(client_player.box.max, a); \
			GETS(box.min, a) = ceil(GETF(old_pos, a) + offset + 0.5f); \
			GETS(box.max, a) = floor(GETF(client_player.pos, a) + offset + 0.5f); \
		} else { \
			dir = -1; \
			offset = GETF(client_player.box.min, a); \
			GETS(box.min, a) = floor(GETF(old_pos, a) + offset - 0.5f); \
			GETS(box.max, a) = ceil(GETF(client_player.pos, a) + offset - 0.5f); \
		} \
		GETS(box.max, a) += dir; \
		for (s32 a = GETS(box.min, a); a != GETS(box.max, a); a += dir) { \
			for (s32 b = GETS(box.min, b); b <= GETS(box.max, b); b++) { \
				for (s32 c = GETS(box.min, c); c <= GETS(box.max, c); c++) { \
					if (is_solid(x, y, z)) { \
						GETF(client_player.pos, a) = (f32) a - offset - 0.5f * (f32) dir; \
						GETF(client_player.velocity, a) = 0.0f; \
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

	if (old_pos.x != client_player.pos.x || old_pos.y != client_player.pos.y || old_pos.z != client_player.pos.z)
		update_pos();
}
