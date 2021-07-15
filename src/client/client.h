#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdbool.h>
#include <pthread.h>
#include "client/client_commands.h"
#include "client/scene.h"
#include "server/server_commands.h"
#include "map.h"
#include "network.h"
#include "types.h"

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
} Client;

void client_disconnect(bool send, const char *detail);
void client_send_position(v3f pos);

#endif
