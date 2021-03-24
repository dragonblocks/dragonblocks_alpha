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
	Map *map;
	List clients;
} Server;

typedef struct Client
{
	int fd;
	char *name;
	char *address;
	Server *server;
	ClientState state;
	pthread_mutex_t *write_mtx;
	pthread_mutex_t mutex;
} Client;

typedef enum
{
	DISCO_NO_REMOVE = 0x01,
	DISCO_NO_SEND = 0x02,
	DISCO_NO_MESSAGE = 0x04,
} DiscoFlag;

void server_disconnect_client(Client *client, int flags, const char *detail);
void server_shutdown(Server *srv);

#endif
