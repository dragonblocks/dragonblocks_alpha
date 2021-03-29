#ifndef _MAPGEN_H_
#define _MAPGEN_H_

#include "server.h"
#include "map.h"

void servermap_init(Server *srv);

void servermap_add_client(Client *client);

#endif
