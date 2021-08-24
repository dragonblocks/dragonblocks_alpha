#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "server/server.h"
#include "server/server_map.h"
#include "signal_handlers.h"
#include "util.h"

static Server server;

// include handle_packets implementation

#include "network.c"

// utility functions

// pthread start routine for reciever thread
static void *reciever_thread(void *arg)
{
	Client *client = arg;

	if (! handle_packets(client))
		server_disconnect_client(client, DISCO_NO_SEND | DISCO_NO_JOIN, "network error");

	return NULL;
}

// accept a new connection, initialize client and start reciever thread
static void accept_client()
{
	struct sockaddr_storage client_address = {0};
	socklen_t client_addrlen = sizeof(client_address);

	int fd = accept(server.sockfd, (struct sockaddr *) &client_address, &client_addrlen);

	if (fd == -1) {
		if (errno != EINTR)
			perror("accept");
		return;
	}

	Client *client = malloc(sizeof(Client));
	client->fd = fd;
	pthread_mutex_init(&client->mtx, NULL);
	client->state = CS_CREATED;
	client->address = address_string((struct sockaddr_in6 *) &client_address);
	client->name = client->address;
	client->server = &server;
	client->pos = (v3f64) {0.0f, 0.0f, 0.0f};
	pthread_create(&client->net_thread, NULL, &reciever_thread, client);

	pthread_rwlock_wrlock(&server.clients_rwlck);
	list_put(&server.clients, client, NULL);
	pthread_rwlock_unlock(&server.clients_rwlck);

	printf("Connected %s\n", client->address);
}

// list_clear_func callback used on server shutdown to disconnect all clients properly
static void list_disconnect_client(void *key, unused void *value, unused void *arg)
{
	server_disconnect_client(key, DISCO_NO_REMOVE | DISCO_NO_MESSAGE, "");
}

// start up the server after socket was created, then accept connections until interrupted, then shutdown server
static void server_run(int fd)
{
	server.config.simulation_distance = 10;

	server.sockfd = fd;
	pthread_rwlock_init(&server.clients_rwlck, NULL);
	server.clients = list_create(NULL);
	pthread_rwlock_init(&server.players_rwlck, NULL);
	server.players = list_create(&list_compare_string);

	server.db = database_open("world.sqlite");
	server_map_init(&server);

	while (! interrupted)
		accept_client();

	printf("Shutting down\n");

	pthread_rwlock_wrlock(&server.clients_rwlck);
	list_clear_func(&server.clients, &list_disconnect_client, NULL);
	pthread_rwlock_unlock(&server.clients_rwlck);

	pthread_rwlock_wrlock(&server.players_rwlck);
	list_clear(&server.players);
	pthread_rwlock_unlock(&server.players_rwlck);

	pthread_rwlock_destroy(&server.clients_rwlck);
	pthread_rwlock_destroy(&server.players_rwlck);

	shutdown(server.sockfd, SHUT_RDWR);
	close(server.sockfd);

	server_map_deinit();

	sqlite3_close(server.db);

	exit(EXIT_SUCCESS);
}

// public functions

// disconnect a client with various options an an optional detail message (flags: DiscoFlag bitmask)
void server_disconnect_client(Client *client, int flags, const char *detail)
{
	client->state = CS_DISCONNECTED;

	if (! (flags & DISCO_NO_REMOVE)) {
		if (client->name) {
			pthread_rwlock_wrlock(&server.players_rwlck);
			list_delete(&server.players, client->name);
			pthread_rwlock_unlock(&server.players_rwlck);
		}
		pthread_rwlock_wrlock(&server.clients_rwlck);
		list_delete(&server.clients, client);
		pthread_rwlock_unlock(&server.clients_rwlck);
	}

	if (! (flags & DISCO_NO_MESSAGE))
		printf("Disconnected %s %s%s%s\n", client->name, INBRACES(detail));

	if (! (flags & DISCO_NO_SEND))
		send_command(client, CC_DISCONNECT);

	pthread_mutex_lock(&client->mtx);
	close(client->fd);
	pthread_mutex_unlock(&client->mtx);

	if (! (flags & DISCO_NO_JOIN))
		pthread_join(client->net_thread, NULL);

	if (client->name != client->address)
		free(client->name);
	free(client->address);

	pthread_mutex_destroy(&client->mtx);
	free(client);
}

// server entry point
int main(int argc, char **argv)
{
	program_name = argv[0];

	if (argc < 2)
		internal_error("missing port");

	struct addrinfo hints = {
		.ai_family = AF_INET6,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_flags = AI_NUMERICSERV | AI_PASSIVE,
	};

	struct addrinfo *info = NULL;

	int gai_state = getaddrinfo(NULL, argv[1], &hints, &info);

	if (gai_state != 0)
		internal_error(gai_strerror(gai_state));

	int fd = socket(info->ai_family, info->ai_socktype, 0);

	if (fd == -1)
		syscall_error("socket");

	int flag = 1;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1)
		syscall_error("setsockopt");

	if (bind(fd, info->ai_addr, info->ai_addrlen) == -1)
		syscall_error("bind");

	if (listen(fd, 3) == -1)
		syscall_error("listen");

	char *addrstr = address_string((struct sockaddr_in6 *) info->ai_addr);
	printf("Listening on %s\n", addrstr);
	free(addrstr);

	freeaddrinfo(info);

	signal_handlers_init();
	server_run(fd);

	return EXIT_SUCCESS;
}
