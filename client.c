#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include "client.h"
#include "signal.h"
#include "util.h"

#include "network.c"

void client_disconnect(Client *client, bool send, const char *detail)
{
	pthread_mutex_lock(&client->mtx);
	if (client->state != CS_DISCONNECTED) {
		if (send)
			write_u32(client->fd, SC_DISCONNECT);

		client->state = CS_DISCONNECTED;
		printf("Disconnected %s%s%s\n", INBRACES(detail));
		close(client->fd);
	}
	pthread_mutex_unlock(&client->mtx);
}

static void *reciever_thread(void *cliptr)
{
	Client *client = cliptr;

	handle_packets(client);

	if (errno == EINTR)
		client_disconnect(client, true, NULL);
	else
		client_disconnect(client, false, "network error");

	if (client->name)
		free(client->name);

	pthread_mutex_destroy(&client->mtx);

	exit(EXIT_SUCCESS);
	return NULL;
}

static void client_loop(Client *client)
{
	while (client->state != CS_DISCONNECTED) {
		if (client->state == CS_CREATED) {
			printf("Enter name: ");
			fflush(stdout);
			char name[NAME_MAX];
			if (scanf("%s", name) == EOF)
				return;
			client->name = strdup(name);
			pthread_mutex_lock(&client->mtx);
			if (write_u32(client->fd, SC_AUTH) && write(client->fd, client->name, strlen(name) + 1)) {
				client->state = CS_AUTH;
				printf("Authenticating...\n");
			}
			pthread_mutex_unlock(&client->mtx);
		} else if (client->state == CS_ACTIVE) {
			printf("%s: ", client->name);
			fflush(stdout);
			char buffer[BUFSIZ] = {0};
			if (scanf("%s", buffer) == EOF)
				return;
			if (strcmp(buffer, "disconnect") == 0) {
				return;
			} else if (strcmp(buffer, "setnode") == 0) {
				v3s32 pos;
				char node[BUFSIZ] = {0};
				if (scanf("%d %d %d %s", &pos.x, &pos.y, &pos.z, node) == EOF)
					return;
				Node node_type = NODE_INVALID;
				if (strcmp(node, "air") == 0)
					node_type = NODE_AIR;
				else if (strcmp(node, "grass") == 0)
					node_type = NODE_GRASS;
				else if (strcmp(node, "dirt") == 0)
					node_type = NODE_DIRT;
				else if (strcmp(node, "stone") == 0)
					node_type = NODE_STONE;
				if (node_type == NODE_INVALID) {
					printf("Invalid node\n");
				} else {
					pthread_mutex_lock(&client->mtx);
					(void) (write_u32(client->fd, SC_SETNODE) && write_v3s32(client->fd, pos) && write_u32(client->fd, node_type));
					pthread_mutex_unlock(&client->mtx);
				}
			} else if (strcmp(buffer, "getnode") == 0) {
				v3s32 pos;
				if (scanf("%d %d %d", &pos.x, &pos.y, &pos.z) == EOF)
					return;
				pthread_mutex_lock(&client->mtx);
				(void) (write_u32(client->fd, SC_GETBLOCK) && write_v3s32(client->fd, map_node_to_block_pos(pos, NULL)));
				pthread_mutex_unlock(&client->mtx);
			} else if (strcmp(buffer, "printnode") == 0) {
				v3s32 pos;
				if (scanf("%d %d %d", &pos.x, &pos.y, &pos.z) == EOF)
					return;
				MapNode node = map_get_node(client->map, pos);
				const char *nodename;
				switch (node.type) {
					case NODE_UNLOADED:
						nodename = "unloaded";
						break;
					case NODE_AIR:
						nodename = "air";
						break;
					case NODE_GRASS:
						nodename = "grass";
						break;
					case NODE_DIRT:
						nodename = "dirt";
						break;
					case NODE_STONE:
						nodename = "stone";
						break;
					case NODE_INVALID:
						nodename = "invalid";
						break;
				}
				printf("%s\n", nodename);
			} else if (strcmp(buffer, "kick") == 0) {
				char target_name[NAME_MAX];
				if (scanf("%s", target_name) == EOF)
					return;
				pthread_mutex_lock(&client->mtx);
				(void) (write_u32(client->fd, SC_KICK) && write(client->fd, target_name, strlen(target_name) + 1) != -1);
				pthread_mutex_unlock(&client->mtx);
			} else {
				printf("Invalid command: %s\n", buffer);
			}
		} else {
			sched_yield();
		}
	}
}

int main(int argc, char **argv)
{
	program_name = argv[0];

	if (argc < 3)
		internal_error("missing address or port");

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,			// support both IPv4 and IPv6
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_flags = AI_NUMERICSERV,
	};

	struct addrinfo *info = NULL;

	int gai_state = getaddrinfo(argv[1], argv[2], &hints, &info);

	if (gai_state != 0)
		internal_error(gai_strerror(gai_state));

	Client client = {
		.fd = -1,
		.map = NULL,
		.name = NULL,
		.state = CS_CREATED,
	};

	pthread_mutex_init(&client.mtx, NULL);

	client.fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

	if (client.fd == -1)
		syscall_error("socket");

	if (connect(client.fd, info->ai_addr, info->ai_addrlen) == -1)
		syscall_error("connect");

	freeaddrinfo(info);

	init_signal_handlers();

	client.map = map_create();

	pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, &reciever_thread, &client);

	client_loop(&client);

	client_disconnect(&client, true, NULL);

	pthread_join(recv_thread, NULL);
}
