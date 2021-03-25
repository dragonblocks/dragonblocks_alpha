#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdbool.h>
#include <pthread.h>
#include "servercommands.h"
#include "clientcommands.h"
#include "network.h"
#include "map.h"
#include "scene.h"

typedef struct Client
{
	int fd;
	pthread_mutex_t mtx;
	ClientState state;
	char *name;
	Map *map;
	Scene *scene;
} Client;

void client_disconnect(bool send, const char *detail);

#endif
