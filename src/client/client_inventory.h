#ifndef _CLIENT_INVENTORY_H_
#define _CLIENT_INVENTORY_H_

#include <stdbool.h>
#include "client/client_player.h"
#include "client/model.h"

void client_inventory_init();
void client_inventory_deinit();
void client_inventory_update();

void client_inventory_depth_offset(f32 offset);
void client_inventory_init_player(ClientEntity *entity);
void client_inventory_deinit_player(ClientEntity *entity);
void client_inventory_update_player(void *peer, ToClientPlayerInventory *pkt);

#endif // _CLIENT_INVENTORY_H_
