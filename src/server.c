#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "server.h"
#include "servermap.h"
#include "signal.h"
#include "util.h"

Server server;

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

#include "network.c"

static void *server_reciever_thread(void *cli)
{
	Client *client = cli;

	if (! handle_packets(client))
		server_disconnect_client(client, DISCO_NO_SEND | DISCO_NO_JOIN, "network error");

	return NULL;
}

static void server_accept_client()
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
	client->pos = (v3f) {0.0f, 0.0f, 0.0f};
	pthread_create(&client->net_thread, NULL, &server_reciever_thread, client);
	client->sent_blocks = list_create(NULL);

	pthread_rwlock_wrlock(&server.clients_rwlck);
	list_put(&server.clients, client, NULL);
	pthread_rwlock_unlock(&server.clients_rwlck);

	printf("Connected %s\n", client->address);
}

static void list_disconnect_client(void *key, __attribute__((unused)) void *value, __attribute__((unused)) void *unused)
{
	server_disconnect_client(key, DISCO_NO_REMOVE | DISCO_NO_MESSAGE, "");
}

void server_start(int fd)
{
	server.sockfd = fd;
	server.map = map_create(NULL);
	pthread_rwlock_init(&server.clients_rwlck, NULL);
	server.clients = list_create(NULL);
	pthread_rwlock_init(&server.players_rwlck, NULL);
	server.players = list_create(&list_compare_string);

	servermap_init(&server);

	while (! interrupted)
		server_accept_client();

	printf("Shutting down\n");

	servermap_deinit();

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

	map_delete(server.map);

	exit(EXIT_SUCCESS);
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

	init_signal_handlers();
	server_start(fd);

	return EXIT_SUCCESS;
}
