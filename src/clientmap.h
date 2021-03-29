#ifndef _CLIENTMAP_H_
#define _CLIENTMAP_H_

#include "client.h"

void clientmap_init(Client *cli);
void clientmap_deinit();

void clientmap_block_changed(MapBlock *block);

#endif
