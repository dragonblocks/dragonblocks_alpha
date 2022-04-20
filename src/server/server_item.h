#ifndef _SERVER_ITEM_H_
#define _SERVER_ITEM_H_

#include <stdbool.h>
#include "item.h"
#include "server/server_player.h"
#include "types.h"

typedef struct {
	void (*use)(ServerPlayer *player, ItemStack *stack, bool pointed, v3s32 pos);
} ServerItemDef;

extern ServerItemDef server_item_defs[];

#endif // _SERVER_ITEM_H_
