#ifndef _SERVERMAP_H_
#define _SERVERMAP_H_

#include "server.h"

typedef struct
{
	List clients;
	char *data;
	size_t size;
} MapBlockExtraData;

void servermap_init(Server *srv);
void servermap_deinit();

#endif
