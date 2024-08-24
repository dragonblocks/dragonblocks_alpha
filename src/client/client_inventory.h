#ifndef _CLIENT_INVENTORY_H_
#define _CLIENT_INVENTORY_H_

#include <stdbool.h>
#include "client/client_player.h"
#include "client/model.h"

typedef enum {
	MENU_UP,
	MENU_DOWN,
	MENU_LEFT,
	MENU_RIGHT,
} MenuDir;

void client_inventory_init();
void client_inventory_deinit();
void client_inventory_update(float dtime);

void client_inventory_depth_offset(f32 offset);
void client_inventory_init_player(ClientEntity *entity);
void client_inventory_deinit_player(ClientEntity *entity);
void client_inventory_update_player(void *peer, ToClientPlayerInventory *pkt);

void client_inventory_set_open(bool open);
void client_inventory_navigate(MenuDir dir);
void client_inventory_select();

#endif // _CLIENT_INVENTORY_H_
