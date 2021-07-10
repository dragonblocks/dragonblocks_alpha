#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdbool.h>
#include <pthread.h>
#include "servercommands.h"
#include "clientcommands.h"
#include "clientplayer.h"
#include "map.h"
#include "network.h"
#include "scene.h"

#ifdef RELEASE
	#define RESSOURCEPATH ""
#else
	#define RESSOURCEPATH "../"
#endif

typedef struct Client
{
	int fd;
	pthread_mutex_t mtx;
	ClientState state;
	char *name;
	Map *map;
	Scene *scene;
	ClientPlayer player;
} Client;

void client_disconnect(bool send, const char *detail);

#endif
