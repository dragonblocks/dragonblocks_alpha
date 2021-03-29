#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>
#include <netinet/in.h>
#include "clientcommands.h"
#include "servercommands.h"
#include "list.h"
#include "map.h"
#include "network.h"

typedef struct
{
	int sockfd;
	List clients;
	Map *map;
} Server;

typedef struct Client
{
	int fd;
	pthread_mutex_t mtx;
	ClientState state;
	char *address;
	char *name;
	Server *server;
	pthread_t thread;
	v3f pos;
} Client;

typedef enum
{
	DISCO_NO_REMOVE = 0x01,
	DISCO_NO_SEND = 0x02,
	DISCO_NO_MESSAGE = 0x04,
} DiscoFlag;

void server_disconnect_client(Client *client, int flags, const char *detail);
void server_shutdown();

#endif
