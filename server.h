#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>
#include "client_commands.h"
#include "linkedlist.h"
#include "map.h"

typedef struct
{
	int sockfd;
	Map *map;
	LinkedList clients;
} Server;

typedef enum
{
	CS_CREATED = 0x01,
	CS_ACTIVE = 0x02,
	CS_DISCONNECTED = 0x04,
} ClientState;

typedef struct
{
	char *name;
	Server *server;
	ClientState state;
	int fd;
	pthread_mutex_t mtx;
} Client;

typedef enum
{
	DISCO_NO_REMOVE = 0x01,
	DISCO_NO_SEND = 0x02,
	DISCO_NO_MESSAGE = 0x04,
} DiscoFlag;

bool server_send_command(Client *client, ClientCommand command);
char *server_get_client_name(Client *client);
void server_disconnect_client(Client *client, int flags, const char *detail);
void server_shutdown(Server *srv);

#endif
