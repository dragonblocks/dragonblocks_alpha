#include <stdio.h>
#include "environment.h"
#include "client/camera.h"
#include "client/client.h"
#include "client/client_map.h"
#include "client/client_player.h"
#include "client/cube.h"
#include "client/debug_menu.h"
#include "client/texture.h"

struct ClientPlayer client_player;

// to be called whenever the player position changes
// rwlock has to be read or write locked
static void update_pos()
{
	camera_set_position((v3f32) {client_player.pos.x, client_player.pos.y + client_player.eye_height, client_player.pos.z});
	client_send_position(client_player.pos);

	client_player.obj->pos = (v3f32) {client_player.pos.x, client_player.pos.y, client_player.pos.z};
	object_transform(client_player.obj);

	debug_menu_update_pos();
	debug_menu_update_humidity();
	debug_menu_update_temperature();
}

// get absolute player bounding box
// rwlock has to be read- or write locked
static aabb3f64 get_box()
{
	return (aabb3f64) {
		{client_player.box.min.x + client_player.pos.x, client_player.box.min.y + client_player.pos.y, client_player.box.min.z + client_player.pos.z},
		{client_player.box.max.x + client_player.pos.x, client_player.box.max.y + client_player.pos.y, client_player.box.max.z + client_player.pos.z},
	};
}

// get absolute integer box that contains all nodes a float bounding box touches
static aabb3s32 round_box(aabb3f64 box)
{
	return (aabb3s32) {
		{floor(box.min.x + 0.5), floor(box.min.y + 0.5), floor(box.min.z + 0.5)},
		{ceil(box.max.x - 0.5), ceil(box.max.y - 0.5), ceil(box.max.z - 0.5)},
	};
}

// return true if node at x, y, z is solid (or unloaded)
static bool is_solid(s32 x, s32 y, s32 z)
{
	Node node = map_get_node(client_map.map, (v3s32) {x, y, z}).type;
	return node == NODE_UNLOADED || node_definitions[node].solid;
}

// determine if player can jump currently (must be standing on a solid block)
// rwlock has to be read- or write locked
static bool can_jump()
{
	if (client_player.velocity.y != 0.0)
		return false;

	aabb3f64 fbox = get_box();
	fbox.min.y -= 0.5;

	aabb3s32 box = round_box(fbox);

	if (fbox.min.y - (f64) box.min.y > 0.01)
		return false;

	for (s32 x = box.min.x; x <= box.max.x; x++)
		for (s32 z = box.min.z; z <= box.max.z; z++)
			if (is_solid(x, box.min.y, z))
				return true;

	return false;
}

// ClientPlayer singleton constructor
void client_player_init()
{
	client_player.pos = (v3f64) {-2500.0, 48.0, -2000.0};
	client_player.velocity = (v3f64) {0.0, 0.0, 0.0};
	client_player.box = (aabb3f64) {{-0.3, 0.0, -0.3}, {0.3, 1.75, 0.3}};
	client_player.yaw = client_player.pitch = 0.0f;
	client_player.eye_height = 1.5;
	client_player.fly = false;
	client_player.collision = true;
	pthread_rwlock_init(&client_player.rwlock, NULL);
}

// ClientPlayer singleton destructor
void client_player_deinit()
{
	pthread_rwlock_destroy(&client_player.rwlock);
}

// create mesh object and info hud
void client_player_add_to_scene()
{
	client_player.obj = object_create();
	client_player.obj->scale = (v3f32) {0.6, 1.75, 0.6};
	client_player.obj->visible = false;

	object_set_texture(client_player.obj, texture_get(RESSOURCEPATH "textures/player.png"));

	for (int f = 0; f < 6; f++) {
		for (int v = 0; v < 6; v++) {
			Vertex3D vertex = cube_vertices[f][v];
			vertex.position.y += 0.5;
			object_add_vertex(client_player.obj, &vertex);
		}
	}

	pthread_rwlock_rdlock(&client_player.rwlock);
	update_pos();
	pthread_rwlock_unlock(&client_player.rwlock);

	debug_menu_update_yaw();
	debug_menu_update_pitch();
}

// jump if possible
void client_player_jump()
{
	pthread_rwlock_wrlock(&client_player.rwlock);
	if (can_jump())
		client_player.velocity.y += 10.0;
	pthread_rwlock_unlock(&client_player.rwlock);
}

// get position (thread-safe)
v3f64 client_player_get_position()
{
	v3f64 pos;

	pthread_rwlock_rdlock(&client_player.rwlock);
	pos = client_player.pos;
	pthread_rwlock_unlock(&client_player.rwlock);

	return pos;
}

// to be called every frame
void client_player_tick(f64 dtime)
{
	pthread_rwlock_wrlock(&client_player.rwlock);

	v3f64 old_pos = client_player.pos;
	v3f64 old_velocity = client_player.velocity;

	if (! client_player.fly)
		client_player.velocity.y -= 32.0 * dtime;

#define GETS(vec, comp) *(s32 *) ((char *) &vec + offsetof(v3s32, comp))
#define GETF(vec, comp) *(f64 *) ((char *) &vec + offsetof(v3f64, comp))
#define PHYSICS(a, b, c) { \
		f64 v = (GETF(client_player.velocity, a) + GETF(old_velocity, a)) / 2.0f; \
		if (v == 0.0) \
			goto a ## _physics_done; \
		aabb3s32 box = round_box(get_box()); \
		v3f64 old_pos = client_player.pos; \
		GETF(client_player.pos, a) += v * dtime; \
		if (! client_player.collision) \
			goto a ## _physics_done; \
		s32 dir; \
		f64 offset; \
		if (v > 0.0) { \
			dir = +1; \
			offset = GETF(client_player.box.max, a); \
			GETS(box.min, a) = ceil(GETF(old_pos, a) + offset + 0.5); \
			GETS(box.max, a) = floor(GETF(client_player.pos, a) + offset + 0.5); \
		} else { \
			dir = -1; \
			offset = GETF(client_player.box.min, a); \
			GETS(box.min, a) = floor(GETF(old_pos, a) + offset - 0.5); \
			GETS(box.max, a) = ceil(GETF(client_player.pos, a) + offset - 0.5); \
		} \
		GETS(box.max, a) += dir; \
		for (s32 a = GETS(box.min, a); a != GETS(box.max, a); a += dir) { \
			for (s32 b = GETS(box.min, b); b <= GETS(box.max, b); b++) { \
				for (s32 c = GETS(box.min, c); c <= GETS(box.max, c); c++) { \
					if (is_solid(x, y, z)) { \
						GETF(client_player.pos, a) = (f64) a - offset - 0.5 * (f64) dir; \
						GETF(client_player.velocity, a) = 0.0; \
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

	if (! v3f64_equals(old_pos, client_player.pos))
		update_pos();

	pthread_rwlock_unlock(&client_player.rwlock);
}
