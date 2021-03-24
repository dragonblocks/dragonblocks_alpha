#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdbool.h>
#include <pthread.h>
#include "servercommands.h"
#include "clientcommands.h"
#include "network.h"
#include "map.h"

typedef struct Client
{
	int fd;
	char *name;
	Map *map;
	ClientState state;
	pthread_mutex_t mtx;
} Client;

void client_disconnect(Client *client, bool send, const char *detail);

#endif
