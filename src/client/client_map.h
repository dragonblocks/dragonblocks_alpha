#ifndef _CLIENT_MAP_H_
#define _CLIENT_MAP_H_

#include "map.h"

void client_map_init(Map *map);
void client_map_start_meshgen();
void client_map_deinit();
void client_map_block_changed(MapBlock *block);

#endif
