#ifndef _SERVER_MAP_H_
#define _SERVER_MAP_H_

#include "server/server.h"

typedef struct
{
	List clients;
	char *data;
	size_t size;
} MapBlockExtraData;

void server_map_init(Server *server);
void server_map_deinit();

#endif
