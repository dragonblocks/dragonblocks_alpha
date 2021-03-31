#ifndef _SERVERMAP_H_
#define _SERVERMAP_H_

#include "server.h"

void servermap_init(Server *srv);

void servermap_add_client(Client *client);

#endif
