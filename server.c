#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include "map.h"
#include "util.h"

typedef struct
{
	int sockfd;
	Map *map;
} Server;

void server_shutdown(Server *srv)
{
	printf("Shutting down\n");
	shutdown(srv->sockfd, SHUT_RDWR);
	close(srv->sockfd);

	map_delete(srv->map);
}

int server_accept_client(Server *srv)
{
	struct sockaddr_in cli_addr_buf = {0};
	socklen_t cli_addrlen_buf = 0;

	int fd = accept(srv->sockfd, (struct sockaddr *) &cli_addr_buf, &cli_addrlen_buf);

	if (fd == -1 && errno != EINTR)
		syscall_error("accept");

	return fd;
}

static void interrupt_handler(int sig)
{
	fprintf(stderr, "%s\n", strsignal(sig));
}

int main(int argc, char **argv)
{
	program_name = argv[0];

	Server server = {
		.sockfd = -1,
		.map = NULL,
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

	struct sigaction sigact = {0};
	sigact.sa_handler = &interrupt_handler;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);

	server.map = map_create(NULL);
	map_set_node(server.map, (v3s32) {0, 0, 0}, map_node_create(NODE_STONE));

	int fd;

	while ((fd = server_accept_client(&server)) != -1) {
		MapBlock *block = map_get_block(server.map, (v3s32) {0, 0, 0}, false);
		assert(block);
		map_serialize_block(fd, block);
		close(fd);
	}

	server_shutdown(&server);
}
