#ifndef _SERVER_H_
#define _SERVER_H_

#include <pthread.h>
#include <netinet/in.h>
#include <dragontype/number.h>
#include <dragontype/list.h>
#include "client/client_commands.h"
#include "server/server_commands.h"
#include "network.h"

typedef struct
{
	int sockfd;                     // TCP socket to accept new connections
	pthread_rwlock_t clients_rwlck; // lock to protect client list
	List clients;                   // Client * -> NULL map with all connected clients
	pthread_rwlock_t players_rwlck; // lock to protect player list
	List players;                   // char * -> Client * map with clients that have finished auth
} Server;

typedef struct Client
{
	int fd;               // TCP socket for connection
	pthread_mutex_t mtx;  // mutex to protect socket
	ClientState state;    // state of the client (created, auth, active, disconnected)
	char *address;        // address string to use as identifier for log messages until auth is completed
	char *name;           // player name (must be unique)
	Server *server;       // pointer to server object (essentially the same for all clients)
	pthread_t net_thread; // reciever thread ID
	v3f64 pos;            // player position
} Client;

typedef enum
{
	DISCO_NO_REMOVE = 0x01,     // don't remove from client and player list (to save extra work on server shutdown)
	DISCO_NO_SEND = 0x02,       // don't notfiy client about the disconnect (if client sent disconnect themselves or the TCP connection died)
	DISCO_NO_MESSAGE = 0x04,    // don't log a message about the disconnect (used on server shutdown)
	DISCO_NO_JOIN = 0x08,       // don't wait for the reciever thread to finish (if TCP connection death was reported by reciever thread, the thread is already dead)
} DiscoFlag;

void server_disconnect_client(Client *client, int flags, const char *detail); // disconnect a client with various options an an optional detail message (flags: DiscoFlag bitmask)

#endif
