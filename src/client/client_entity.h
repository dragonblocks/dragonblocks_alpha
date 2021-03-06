#ifndef _CLIENT_ENTITY_H_
#define _CLIENT_ENTITY_H_

#include <dragonstd/refcount.h>
#include <pthread.h>
#include "client/gui.h"
#include "client/model.h"
#include "common/entity.h"
#include "common/item.h"
#include "types.h"

typedef struct {
	EntityData data;
	struct ClientEntityType *type;
	Refcount rc;

	Model *model;
	GUIElement *nametag;
	void *extra;

	aabb3f32 box_collision;
	aabb3f32 box_culling;   // ToDo
	mat4x4 *nametag_offset;

	pthread_rwlock_t lock_pos_rot;
	pthread_rwlock_t lock_nametag;
	pthread_rwlock_t lock_box_off;
} ClientEntity;

// Entity is pronounced N-Tiddy, hmmmm...

typedef struct ClientEntityType {
	void (*add   )(ClientEntity *entity); // called when server sent addition of entity
	void (*remove)(ClientEntity *entity); // called when server sent removal of entity
	void (*free  )(ClientEntity *entity); // called when entity is garbage collected

	void (*update_pos_rot)(ClientEntity *entity);
	void (*update_nametag)(ClientEntity *entity);

	void (*transform)(ClientEntity *entity);
} ClientEntityType;

extern ClientEntityType client_entity_types[];
extern Mesh client_entity_cube;
extern ModelShader client_entity_shader;

void client_entity_init();
void client_entity_deinit();

void client_entity_gfx_init();
void client_entity_gfx_deinit();
void client_entity_gfx_update();
void client_entity_depth_offset(f32 offset);

ClientEntity *client_entity_grab(u64 id);
void client_entity_drop(ClientEntity *entity);

void client_entity_transform(ClientEntity *entity);

void client_entity_add(void *peer, ToClientEntityAdd *pkt);
void client_entity_remove(void *peer, ToClientEntityRemove *pkt);
void client_entity_update_pos_rot(void *peer, ToClientEntityUpdatePosRot *pkt);
void client_entity_update_nametag(void *peer, ToClientEntityUpdateNametag *pkt);

#endif // _CLIENT_ENTITY_H_
