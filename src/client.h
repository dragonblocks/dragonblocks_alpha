#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdbool.h>
#include <pthread.h>
#include "servercommands.h"
#include "clientcommands.h"
#include "network.h"
#include "list.h"
#include "map.h"

#define CLIENT_MTX_COUNT 3

typedef struct Client
{
	int fd;
	char *name;
	Map *map;
	ClientState state;
	pthread_mutex_t *write_mtx;
	pthread_mutex_t *meshlist_mtx;
	pthread_mutex_t *mapblock_meshgen_mtx;
	pthread_mutex_t mutexes[CLIENT_MTX_COUNT];
	List meshlist;
	List mapblock_meshgen_queue;
} Client;

void client_add_mesh(Client *client, Mesh *mesh);
void client_remove_mesh(Client *client, Mesh *mesh);
void client_mapblock_changed(Client *client, v3s32 pos);
void client_disconnect(Client *client, bool send, const char *detail);

#endif
