#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "client/client.h"
#include "client/client_map.h"
#include "client/client_player.h"
#include "client/game.h"
#include "client/input.h"
#include "signal_handlers.h"
#include "util.h"

static Client client;

void client_disconnect(bool send, const char *detail)
{
	pthread_mutex_lock(&client.mtx);
	if (client.state != CS_DISCONNECTED) {
		if (send)
			write_u32(client.fd, SC_DISCONNECT);

		client.state = CS_DISCONNECTED;
		printf("Disconnected %s%s%s\n", INBRACES(detail));
		close(client.fd);
	}
	pthread_mutex_unlock(&client.mtx);
}

void client_send_position(v3f64 pos)
{
	pthread_mutex_lock(&client.mtx);
	(void) (write_u32(client.fd, SC_POS) && write_v3f64(client.fd, pos));
	pthread_mutex_unlock(&client.mtx);
}

#include "network.c"

static void *reciever_thread(unused void *arg)
{
	handle_packets(&client);

	if (errno != EINTR)
		client_disconnect(false, "network error");

	return NULL;
}

static bool client_name_prompt()
{
	printf("Enter name: ");
	fflush(stdout);
	char name[PLAYER_NAME_MAX];
	if (scanf("%s", name) == EOF)
		return false;
	client.name = strdup(name);
	pthread_mutex_lock(&client.mtx);
	if (write_u32(client.fd, SC_AUTH) && write(client.fd, client.name, strlen(client.name) + 1)) {
		client.state = CS_AUTH;
		printf("Authenticating...\n");
	}
	pthread_mutex_unlock(&client.mtx);
	return true;
}

static bool client_authenticate()
{
	for ever {
		switch (client.state) {
			case CS_CREATED:
				if (client_name_prompt())
					break;
				else
					return false;
			case CS_AUTH:
				if (interrupted)
					return false;
				else
					sched_yield();
				break;
			case CS_ACTIVE:
				return true;
			case CS_DISCONNECTED:
				return false;
		}
	}
	return false;
}

static void client_start(int fd)
{
	client.fd = fd;
	pthread_mutex_init(&client.mtx, NULL);
	client.state = CS_CREATED;
	client.name = NULL;

	client_map_init(&client);
	client_player_init();

	pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, &reciever_thread, NULL);

	if (client_authenticate())
		game(&client);

	if (client.state != CS_DISCONNECTED)
		client_disconnect(true, NULL);

	if (client.name)
		free(client.name);

	pthread_join(recv_thread, NULL);

	client_player_deinit();
	client_map_deinit();

	pthread_mutex_destroy(&client.mtx);
}

int main(int argc, char **argv)
{
	program_name = argv[0];

	if (argc < 3)
		internal_error("missing address or port");

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_flags = AI_NUMERICSERV,
	};

	struct addrinfo *info = NULL;

	int gai_state = getaddrinfo(argv[1], argv[2], &hints, &info);

	if (gai_state != 0)
		internal_error(gai_strerror(gai_state));

	int fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

	if (fd == -1)
		syscall_error("socket");

	if (connect(fd, info->ai_addr, info->ai_addrlen) == -1)
		syscall_error("connect");

	char *addrstr = address_string((struct sockaddr_in6 *) info->ai_addr);
	printf("Connected to %s\n", addrstr);
	free(addrstr);

	freeaddrinfo(info);

	signal_handlers_init();
	client_start(fd);

	return EXIT_SUCCESS;
}
