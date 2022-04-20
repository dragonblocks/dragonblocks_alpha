#include <stdio.h>
#include <stdlib.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_inventory.h"
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

static Model *player_model;

// updat epos/rot box/eye functions

static void update_camera()
{
	vec4 dst, src = {0.0f, 0.0f, 0.0f, 1.0f};

	ClientPlayerData *data = player_entity->extra;

	if (data->bones.eyes)
		mat4x4_mul_vec4(dst, data->bones.eyes->abs, src);
	else
		vec4_dup(dst, src);

	camera_set_position((v3f32) {dst[0], dst[1], dst[2]});
}

static void update_pos()
{
	debug_menu_changed(ENTRY_POS);
	debug_menu_changed(ENTRY_HUMIDITY);
	debug_menu_changed(ENTRY_TEMPERATURE);
}

static void update_rot()
{
	camera_set_angle(M_PI / 2 - player_entity->data.rot.y, -player_entity->data.rot.x);
	debug_menu_changed(ENTRY_YAW);
	debug_menu_changed(ENTRY_PITCH);
}

static void update_transform()
{
	client_entity_transform(player_entity);
	update_camera();
}

static void send_pos_rot()
{
	update_transform();

	dragonnet_peer_send_ToServerPosRot(client, &(ToServerPosRot) {
		.pos = player_entity->data.pos,
		.rot = player_entity->data.rot,
	});
}

static void recv_pos_rot()
{
	update_transform();

	update_pos();
	update_rot();
}

// entity callbacks

static void local_on_model_before_render(__attribute__((unused)) Model *model)
{
	client_entity_depth_offset(0.2f);
	client_inventory_depth_offset(0.2f);
}

static void local_on_model_after_render(__attribute__((unused)) Model *model)
{
	client_entity_depth_offset(0.0f);
	client_inventory_depth_offset(0.0f);}

static void on_add(ClientEntity *entity)
{
	entity->model = model_clone(player_model);
	entity->model->extra = refcount_grb(&entity->rc);

	ClientPlayerData *data = entity->extra = malloc(sizeof *data);
	data->bones = (struct ClientPlayerBones) {NULL};

	model_get_bones(entity->model, (ModelBoneMapping[9]) {
		{"nametag",        &data->bones.nametag   },
		{"neck",           &data->bones.neck      },
		{"neck.head.eyes", &data->bones.eyes      },
		{"arm_left",       &data->bones.arm_left  },
		{"arm_right",      &data->bones.arm_right },
		{"arm_left.hand",  &data->bones.hand_left },
		{"arm_right.hand", &data->bones.hand_right},
		{"leg_left",       &data->bones.leg_left  },
		{"leg_right",      &data->bones.leg_right },
	}, 9);

	entity->nametag_offset = data->bones.nametag ? &data->bones.nametag->abs : NULL;
	entity->box_collision = (aabb3f32) {{-0.45f, 0.0f, -0.45f}, {0.45f, 1.8f, 0.45f}};

	client_inventory_init_player(entity);

	model_scene_add(entity->model);
	client_entity_transform(entity);
}

static void on_remove(ClientEntity *entity)
{
	entity->model->flags.delete = 1;
	entity->model = NULL;
}

static void on_free(ClientEntity *entity)
{
	client_inventory_init_player(entity);
	free(entity->extra);
}

static void on_transform(ClientEntity *entity)
{
	ClientPlayerData *data = entity->extra;

	entity->model->root->rot.x = entity->model->root->rot.z = 0.0f;

	if (data->bones.neck) {
		data->bones.neck->rot.x = entity->data.rot.x;
		model_node_transform(data->bones.neck);
	}
}

static void local_on_add(ClientEntity *entity)
{
	pthread_rwlock_wrlock(&lock_player_entity);

	if (player_entity) {
		fprintf(stderr, "[error] attempt to re-add localplayer entity\n");
		exit(EXIT_FAILURE);
	}

	on_add(entity);
	entity->model->callbacks.before_render = &local_on_model_before_render;
	entity->model->callbacks.after_render = &local_on_model_after_render;

	player_entity = refcount_grb(&entity->rc);
	recv_pos_rot();

	entity->type->update_nametag(entity);

	pthread_rwlock_unlock(&lock_player_entity);
}

static void local_on_remove(ClientEntity *entity)
{
	pthread_rwlock_wrlock(&lock_player_entity);
	refcount_drp(&entity->rc);
	player_entity = NULL;
	pthread_rwlock_unlock(&lock_player_entity);

	on_remove(entity);
}

static void local_on_update_pos_rot(__attribute__((unused)) ClientEntity *entity)
{
	recv_pos_rot();
}

static void local_on_update_nametag(ClientEntity *entity)
{
	if (entity->data.nametag) {
		free(entity->data.nametag);
		entity->data.nametag = NULL;
	}
}

static void on_model_delete(Model *model)
{
	if (model->extra)
		refcount_drp(&((ClientEntity *) model->extra)->rc);
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

	client_entity_types[ENTITY_PLAYER] = (ClientEntityType) {
		.add = &on_add,
		.remove = &on_remove,
		.free = &on_free,
		.update_pos_rot = NULL,
		.update_nametag = NULL,
		.transform = &on_transform,
	};

	client_entity_types[ENTITY_LOCALPLAYER] = (ClientEntityType) {
		.add = &local_on_add,
		.remove = &local_on_remove,
		.free = &on_free,
		.update_pos_rot = &local_on_update_pos_rot,
		.update_nametag = &local_on_update_nametag,
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

void client_player_gfx_init()
{
	player_model = model_load(
		RESSOURCE_PATH "models/player.txt", RESSOURCE_PATH "textures/models/player",
		&client_entity_cube, &client_entity_shader);

	player_model->callbacks.delete = &on_model_delete;
}

void client_player_gfx_deinit()
{
	model_delete(player_model);
}

ClientEntity *client_player_entity(u64 id)
{
	ClientEntity *entity = client_entity_grab(id);

	if (entity->type == &client_entity_types[ENTITY_LOCALPLAYER]
			|| entity->type == &client_entity_types[ENTITY_PLAYER])
		return entity;

	refcount_drp(&entity->rc);
	return NULL;
}

ClientEntity *client_player_entity_local()
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

// jump if possible
void client_player_jump()
{
	ClientEntity *entity = client_player_entity_local();
	if (!entity)
		return;

	pthread_rwlock_rdlock(&entity->lock_pos_rot);
	pthread_rwlock_rdlock(&entity->lock_box_off);

	if (physics_ground(
		client_terrain,
		client_player.movement.collision,
		entity->box_collision,
		&entity->data.pos,
		&client_player.velocity
	))
		client_player.velocity.y += client_player.movement.jump;

	pthread_rwlock_unlock(&entity->lock_box_off);
	pthread_rwlock_unlock(&entity->lock_pos_rot);

	refcount_drp(&entity->rc);
}

// to be called every frame
void client_player_tick(f64 dtime)
{
	ClientEntity *entity = client_player_entity_local();
	if (!entity)
		return;

	pthread_rwlock_rdlock(&client_player.lock_movement);
	pthread_rwlock_wrlock(&entity->lock_pos_rot);
	pthread_rwlock_rdlock(&entity->lock_box_off);

	if (physics_step(
		client_terrain,
		client_player.movement.collision,
		entity->box_collision,
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

	pthread_rwlock_unlock(&entity->lock_box_off);
	pthread_rwlock_unlock(&entity->lock_pos_rot);
	pthread_rwlock_unlock(&client_player.lock_movement);

	refcount_drp(&entity->rc);
}
