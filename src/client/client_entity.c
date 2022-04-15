#include <dragonstd/map.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/client_entity.h"
#include "client/client_player.h"

ClientEntityType client_entity_types[COUNT_ENTITY];

static Map entities;

// any thread
// called when adding, getting or removing an entity from the map
static int cmp_entity(const Refcount *entity, const u64 *id)
{
	return u64_cmp(&((ClientEntity *) entity->obj)->data.id, id);
}

// recv thread
// called when server sent removal of entity
static void entity_drop(ClientEntity *entity)
{
	if (entity->type->remove)
		entity->type->remove(entity);

	refcount_drp(&entity->rc);
}

// any thread
// called when all refs have been dropped
static void entity_delete(ClientEntity *entity)
{
	if (entity->type->free)
		entity->type->free(entity);

	refcount_dst(&entity->rc);

	if (entity->data.nametag)
		free(entity->data.nametag);

	pthread_rwlock_init(&entity->lock_pos_rot, NULL);
	pthread_rwlock_init(&entity->lock_box_eye, NULL);
	pthread_rwlock_init(&entity->lock_nametag, NULL);

	free(entity);
}

// main thread
// called on startup
void client_entity_init()
{
	map_ini(&entities);
}

// main thead
// called on shutdown
void client_entity_deinit()
{
	// forget all entities
	map_cnl(&entities, &refcount_drp, NULL, NULL, 0);
}

ClientEntity *client_entity_grab(u64 id)
{
	return id ? map_get(&entities, &id, &cmp_entity, &refcount_grb) : NULL;
}

void client_entity_transform(ClientEntity *entity)
{
	if (!entity->model)
		return;

	entity->model->root->pos = (v3f32) {entity->data.pos.x, entity->data.pos.y, entity->data.pos.z};
	entity->model->root->rot = (v3f32) {entity->data.rot.x, entity->data.rot.y, entity->data.rot.z};

	if (entity->type->transform)
		entity->type->transform(entity);

	model_node_transform(entity->model->root);
}

void client_entity_add(__attribute__((unused)) DragonnetPeer *peer, ToClientEntityAdd *pkt)
{
	if (pkt->type >= COUNT_ENTITY)
		return;

	ClientEntity *entity = malloc(sizeof *entity);

	entity->data = pkt->data;
	entity->type = &client_entity_types[pkt->type];
	refcount_ini(&entity->rc, entity, &entity_delete);

	pkt->data.nametag = NULL;

	entity->model = NULL;

	pthread_rwlock_init(&entity->lock_pos_rot, NULL);
	pthread_rwlock_init(&entity->lock_box_eye, NULL);
	pthread_rwlock_init(&entity->lock_nametag, NULL);

	if (entity->type->add)
		entity->type->add(entity);

	if (!map_add(&entities, &entity->data.id, &entity->rc, &cmp_entity, &refcount_inc))
		fprintf(stderr, "[warning] failed to add entity %lu\n", entity->data.id);

	refcount_drp(&entity->rc);
}

void client_entity_remove(__attribute__((unused)) DragonnetPeer *peer, ToClientEntityRemove *pkt)
{
	map_del(&entities, &pkt->id, &cmp_entity, &entity_drop, NULL, &refcount_obj);
}

void client_entity_update_pos_rot(__attribute__((unused)) DragonnetPeer *peer, ToClientEntityUpdatePosRot *pkt)
{
	ClientEntity *entity = client_entity_grab(pkt->id);

	if (!entity)
		return;

	pthread_rwlock_wrlock(&entity->lock_pos_rot);

	entity->data.pos = pkt->pos;
	entity->data.rot = pkt->rot;

	if (entity->type->update_pos_rot)
		entity->type->update_pos_rot(entity);

	pthread_rwlock_unlock(&entity->lock_pos_rot);

	refcount_drp(&entity->rc);
}

void client_entity_update_box_eye(__attribute__((unused)) DragonnetPeer *peer, ToClientEntityUpdateBoxEye *pkt)
{
	ClientEntity *entity = client_entity_grab(pkt->id);

	if (!entity)
		return;

	pthread_rwlock_wrlock(&entity->lock_box_eye);

	entity->data.box = pkt->box;
	entity->data.eye = pkt->eye;

	if (entity->type->update_box_eye)
		entity->type->update_box_eye(entity);

	pthread_rwlock_unlock(&entity->lock_box_eye);

	refcount_drp(&entity->rc);
}

void client_entity_update_nametag(__attribute__((unused)) DragonnetPeer *peer, ToClientEntityUpdateNametag *pkt)
{
	ClientEntity *entity = client_entity_grab(pkt->id);

	if (!entity)
		return;

	pthread_rwlock_wrlock(&entity->lock_nametag);

	if (entity->data.nametag)
		free(entity->data.nametag);

	entity->data.nametag = pkt->nametag;
	pkt->nametag = NULL;

	if (entity->type->update_nametag)
		entity->type->update_nametag(entity);

	pthread_rwlock_unlock(&entity->lock_nametag);

	refcount_drp(&entity->rc);
}
