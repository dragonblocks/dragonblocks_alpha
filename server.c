#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <errno.h>
#include <endian.h>
#include <pthread.h>
#include "linkedlist.h"
#include "server.h"
#include "servercommands.h"
#include "signal.h"
#include "util.h"

#include "network.c"

char *server_get_client_name(Client *client)
{
	return client->name ? client->name : "<unauthenticated client>";
}

void server_disconnect_client(Client *client, int flags, const char *detail)
{
	if (client->name && ! (flags & DISCO_NO_REMOVE))
		linked_list_delete(&client->server->clients, client->name);

	if (! (flags & DISCO_NO_MESSAGE))
		printf("Disconnected %s %s%s%s\n", server_get_client_name(client), INBRACES(detail));

	if (! (flags & DISCO_NO_SEND))
		send_command(client, CC_DISCONNECT);

	client->state = CS_DISCONNECTED;

	pthread_mutex_lock(&client->mtx);
	close(client->fd);
	pthread_mutex_unlock(&client->mtx);
}

void server_shutdown(Server *srv)
{
	printf("Shutting down\n");

	ITERATE_LINKEDLIST(&srv->clients, pair) server_disconnect_client(pair->value, DISCO_NO_REMOVE | DISCO_NO_MESSAGE, "");
	linked_list_clear(&srv->clients);

	shutdown(srv->sockfd, SHUT_RDWR);
	close(srv->sockfd);

	map_delete(srv->map);

	exit(EXIT_SUCCESS);
}

static void *reciever_thread(void *clientptr)
{
	Client *client = clientptr;

	handle_packets(client);

	if (client->state != CS_DISCONNECTED)
		server_disconnect_client(client, DISCO_NO_SEND, "network error");

	if (client->name)
		free(client->name);

	pthread_mutex_destroy(&client->mtx);
	free(client);

	return NULL;
}

static void accept_client(Server *srv)
{
	struct sockaddr_in cli_addr_buf = {0};
	socklen_t cli_addrlen_buf = 0;

	int fd = accept(srv->sockfd, (struct sockaddr *) &cli_addr_buf, &cli_addrlen_buf);

	if (fd == -1) {
		if (errno == EINTR)
			server_shutdown(srv);
		else
			syscall_error("accept");
	}

	Client *client = malloc(sizeof(Client));
	client->server = srv;
	client->state = CS_CREATED;
	client->fd = fd;
	client->name = NULL;

	pthread_mutex_init(&client->mtx, NULL);

	pthread_t thread;
	pthread_create(&thread, NULL, &reciever_thread, client);
}

int main(int argc, char **argv)
{
	program_name = argv[0];

	Server server = {
		.sockfd = -1,
		.map = NULL,
		.clients = linked_list_create(),
	};

	server.sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (server.sockfd == -1)
		syscall_error("socket");

	struct sockaddr_in srv_addr = {
		.sin_family = AF_INET,
		.sin_port = get_port_from_args(argc, argv, 1),
		.sin_addr = {.s_addr = INADDR_ANY},
	};

	int flag = 1;

	if (setsockopt(server.sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, 4) == -1)
		syscall_error("setsockopt");

	if (bind(server.sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1)
		syscall_error("bind");

	if (listen(server.sockfd, 3) == -1)
		syscall_error("listen");

	init_signal_handlers();

	server.map = map_create(NULL);

	for ever accept_client(&server);
}
