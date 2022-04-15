#ifndef _CLIENT_ENTITY_H_
#define _CLIENT_ENTITY_H_

#include <dragonnet/peer.h>
#include <dragonstd/refcount.h>
#include <pthread.h>
#include "client/model.h"
#include "entity.h"
#include "types.h"

typedef struct {
	EntityData data;
	struct ClientEntityType *type;
	Refcount rc;

	Model *model;

	pthread_rwlock_t lock_pos_rot;
	pthread_rwlock_t lock_box_eye;
	pthread_rwlock_t lock_nametag;
} ClientEntity;

typedef struct ClientEntityType {
	void (*add   )(ClientEntity *entity); // called when server sent addition of entity
	void (*remove)(ClientEntity *entity); // called when server sent removal of entity
	void (*free  )(ClientEntity *entity); // called when entity is garbage collected

	void (*update_pos_rot)(ClientEntity *entity);
	void (*update_box_eye)(ClientEntity *entity);
	void (*update_nametag)(ClientEntity *entity);

	void (*transform)(ClientEntity *entity);
} ClientEntityType;

void client_entity_init();
void client_entity_deinit();

ClientEntity *client_entity_grab(u64 id);
void client_entity_drop(ClientEntity *entity);

void client_entity_transform(ClientEntity *entity);

void client_entity_add(DragonnetPeer *peer, ToClientEntityAdd *pkt);
void client_entity_remove(DragonnetPeer *peer, ToClientEntityRemove *pkt);
void client_entity_update_pos_rot(DragonnetPeer *peer, ToClientEntityUpdatePosRot *pkt);
void client_entity_update_box_eye(DragonnetPeer *peer, ToClientEntityUpdateBoxEye *pkt);
void client_entity_update_nametag(DragonnetPeer *peer, ToClientEntityUpdateNametag *pkt);

extern ClientEntityType client_entity_types[];

#endif // _CLIENT_ENTITY_H_
