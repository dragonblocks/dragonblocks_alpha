#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>
#include "clientcommands.h"
#include "servercommands.h"
#include "linkedlist.h"
#include "map.h"
#include "network.h"

typedef struct
{
	int sockfd;
	Map *map;
	LinkedList clients;
} Server;

typedef struct Client
{
	int fd;
	char *name;
	Server *server;
	ClientState state;
	pthread_mutex_t mtx;
} Client;

typedef enum
{
	DISCO_NO_REMOVE = 0x01,
	DISCO_NO_SEND = 0x02,
	DISCO_NO_MESSAGE = 0x04,
} DiscoFlag;

char *server_get_client_name(Client *client);
void server_disconnect_client(Client *client, int flags, const char *detail);
void server_shutdown(Server *srv);

#endif
