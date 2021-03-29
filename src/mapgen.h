#ifndef _MAPGEN_H_
#define _MAPGEN_H_

#include "server.h"
#include "map.h"

void mapgen_init(Server *srv);
void mapgen_start_thread(Client *client);

#endif
