#include <stdio.h>
#include <stdlib.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_player.h"
#include "client/client_terrain.h"
#include "client/cube.h"
#include "client/debug_menu.h"
#include "client/texture.h"
#include "environment.h"
#include "physics.h"

struct ClientPlayer client_player;

static ClientEntity *player_entity;
static pthread_rwlock_t lock_player_entity;

// updat epos/rot box/eye functions

static void update_eye_pos_camera()
{
	v3f64 pos = player_entity->data.pos;
	v3f32 eye = player_entity->data.eye;

	camera_set_position((v3f32) {pos.x + eye.x, pos.y + eye.y, pos.z + eye.z});
}

static void update_pos()
{
	pthread_rwlock_rdlock(&player_entity->lock_box_eye);
	update_eye_pos_camera();
	pthread_rwlock_unlock(&player_entity->lock_box_eye);

	debug_menu_changed(ENTRY_POS);
	debug_menu_changed(ENTRY_HUMIDITY);
	debug_menu_changed(ENTRY_TEMPERATURE);
}

static void update_rot()
{
	camera_set_angle(player_entity->data.rot.x, player_entity->data.rot.y);
	debug_menu_changed(ENTRY_YAW);
	debug_menu_changed(ENTRY_PITCH);
}

static void update_transform()
{
	client_entity_transform(player_entity);
}

static void send_pos_rot()
{
	dragonnet_peer_send_ToServerPosRot(client, &(ToServerPosRot) {
		.pos = player_entity->data.pos,
		.rot = player_entity->data.rot,
	});

	update_transform();
}

static void recv_pos_rot()
{
	update_pos();
	update_rot();

	update_transform();
}

// entity callbacks

static void on_add(ClientEntity *entity)
{
	pthread_rwlock_wrlock(&lock_player_entity);

	if (player_entity) {
		fprintf(stderr, "[error] attempt to re-add localplayer entity\n");
		exit(EXIT_FAILURE);
	} else {
		player_entity = refcount_grb(&entity->rc);
		recv_pos_rot();

		entity->type->update_nametag(entity);
	}

	pthread_rwlock_unlock(&lock_player_entity);
}

static void on_remove(ClientEntity *entity)
{
	pthread_rwlock_wrlock(&lock_player_entity);
	refcount_drp(&entity->rc);
	player_entity = NULL;
	pthread_rwlock_unlock(&lock_player_entity);
}

static void on_update_pos_rot(__attribute__((unused)) ClientEntity *entity)
{
	recv_pos_rot();
}

static void on_update_box_eye(__attribute__((unused)) ClientEntity *entity)
{
	pthread_rwlock_rdlock(&lock_player_entity);
	update_eye_pos_camera();
	pthread_rwlock_unlock(&lock_player_entity);
}

static void on_update_nametag(ClientEntity *entity)
{
	if (entity->data.nametag) {
		free(entity->data.nametag);
		entity->data.nametag = NULL;
	}
}

static void on_transform(ClientEntity *entity)
{
	entity->model->root->rot.y = entity->model->root->rot.z = 0.0f;
}

// called on startup
void client_player_init()
{
	client_player.movement = (ToClientMovement) {
		.flight = false,
		.collision = true,
		.speed = 0.0f,
		.jump = 0.0f,
		.gravity = 0.0f,
	};

	client_entity_types[ENTITY_LOCALPLAYER] = (ClientEntityType) {
		.add = &on_add,
		.remove = &on_remove,
		.free = NULL,
		.update_pos_rot = &on_update_pos_rot,
		.update_box_eye = &on_update_box_eye,
		.update_nametag = &on_update_nametag,
		.transform = &on_transform,
	};

	client_entity_types[ENTITY_PLAYER] = (ClientEntityType) {
		.add = NULL,
		.remove = NULL,
		.free = NULL,
		.update_pos_rot = NULL,
		.update_box_eye = NULL,
		.update_nametag = NULL,
		.transform = &on_transform,
	};

	pthread_rwlock_init(&client_player.lock_movement, NULL);

	player_entity = NULL;
	pthread_rwlock_init(&lock_player_entity, NULL);
}

// called on shutdown
void client_player_deinit()
{
	pthread_rwlock_destroy(&client_player.lock_movement);
	pthread_rwlock_destroy(&lock_player_entity);
}

ClientEntity *client_player_entity()
{
	ClientEntity *entity = NULL;

	pthread_rwlock_rdlock(&lock_player_entity);
	if (player_entity)
		entity = refcount_grb(&player_entity->rc);
	pthread_rwlock_unlock(&lock_player_entity);

	return entity;
}

void client_player_update_pos(ClientEntity *entity)
{
	pthread_rwlock_rdlock(&lock_player_entity);

	if (entity == player_entity) {
		update_pos();
		send_pos_rot();
	}

	pthread_rwlock_unlock(&lock_player_entity);
}

void client_player_update_rot(ClientEntity *entity)
{
	pthread_rwlock_rdlock(&lock_player_entity);

	if (entity == player_entity) {
		update_rot();
		send_pos_rot();
	}

	pthread_rwlock_unlock(&lock_player_entity);
}

/*
// create mesh object and info hud
void client_player_add_to_scene()
{
	client_player.obj = object_create();
	client_player.obj->scale = (v3f32) {0.6, 1.75, 0.6};
	client_player.obj->visible = false;

	object_set_texture(client_player.obj, texture_load(RESSOURCE_PATH "textures/player.png", true));

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
*/

// jump if possible
void client_player_jump()
{
	ClientEntity *entity = client_player_entity();
	if (!entity)
		return;

	pthread_rwlock_rdlock(&entity->lock_pos_rot);
	pthread_rwlock_rdlock(&entity->lock_box_eye);

	if (physics_ground(
		client_terrain,
		client_player.movement.collision,
		entity->data.box,
		&entity->data.pos,
		&client_player.velocity
	))
		client_player.velocity.y += client_player.movement.jump;

	pthread_rwlock_unlock(&entity->lock_box_eye);
	pthread_rwlock_unlock(&entity->lock_pos_rot);

	refcount_drp(&entity->rc);
}

// to be called every frame
void client_player_tick(f64 dtime)
{
	ClientEntity *entity = client_player_entity();
	if (!entity)
		return;

	pthread_rwlock_rdlock(&client_player.lock_movement);
	pthread_rwlock_wrlock(&entity->lock_pos_rot);
	pthread_rwlock_rdlock(&entity->lock_box_eye);

	if (physics_step(
		client_terrain,
		client_player.movement.collision,
		entity->data.box,
		&entity->data.pos,
		&client_player.velocity,
		&(v3f64) {
			0.0,
			client_player.movement.flight ? 0.0 : -client_player.movement.gravity,
			0.0,
		},
		dtime
	))
		client_player_update_pos(entity);

	pthread_rwlock_unlock(&entity->lock_box_eye);
	pthread_rwlock_unlock(&entity->lock_pos_rot);
	pthread_rwlock_unlock(&client_player.lock_movement);

	refcount_drp(&entity->rc);
}
