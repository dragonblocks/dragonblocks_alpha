#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "server.h"
#include "signal.h"
#include "util.h"

#include "network.c"

void server_disconnect_client(Client *client, int flags, const char *detail)
{
	client->state = CS_DISCONNECTED;

	if (client->name && ! (flags & DISCO_NO_REMOVE))
		linked_list_delete(&client->server->clients, client->name);

	if (! (flags & DISCO_NO_MESSAGE))
		printf("Disconnected %s %s%s%s\n", client->name, INBRACES(detail));

	if (! (flags & DISCO_NO_SEND))
		send_command(client, CC_DISCONNECT);

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

	if (client->name != client->address)
		free(client->name);

	free(client->address);

	pthread_mutex_destroy(&client->mtx);
	free(client);

	return NULL;
}

static void accept_client(Server *srv)
{
	struct sockaddr_storage client_address = {0};
	socklen_t client_addrlen = sizeof(client_address);

	int fd = accept(srv->sockfd, (struct sockaddr *) &client_address, &client_addrlen);

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
	client->address = address_string((struct sockaddr_in6 *) &client_address);
	client->name = client->address;

	printf("Connected %s\n", client->address);

	pthread_mutex_init(&client->mtx, NULL);

	pthread_t thread;
	pthread_create(&thread, NULL, &reciever_thread, client);
}

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

	Server server = {
		.sockfd = -1,
		.map = NULL,
		.clients = linked_list_create(),
	};

	server.sockfd = socket(info->ai_family, info->ai_socktype, 0);

	if (server.sockfd == -1)
		syscall_error("socket");

	int flag = 1;

	if (setsockopt(server.sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, 4) == -1)
		syscall_error("setsockopt");

	if (bind(server.sockfd, info->ai_addr, info->ai_addrlen) == -1)
		syscall_error("bind");

	if (listen(server.sockfd, 3) == -1)
		syscall_error("listen");

	char *addrstr = address_string((struct sockaddr_in6 *) info->ai_addr);
	printf("Listening on %s\n", addrstr);
	free(addrstr);

	freeaddrinfo(info);

	init_signal_handlers();

	server.map = map_create(NULL);

	for ever accept_client(&server);
}
