#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "map.h"
#include "util.h"

int main(int argc, char **argv)
{
	program_name = argv[0];

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd == -1)
		syscall_error("socket");

	struct sockaddr_in srv_addr = {
		.sin_family = AF_INET,
		.sin_port = get_port_from_args(argc, argv, 1),
		.sin_addr = {.s_addr = INADDR_ANY},
	};

	int flag = 1;

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, 4) == -1)
		syscall_error("setsockopt");

	if (bind(sockfd, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1)
		syscall_error("bind");

	if (listen(sockfd, 3) == -1)
		syscall_error("listen");

	Map *map = map_create(NULL);
	map_set_node(map, (v3s32) {0, 0, 0}, map_node_create(NODE_STONE));

	struct sockaddr_in cli_addr_buf = {0};
	socklen_t cli_addrlen_buf = 0;

	int fd = accept(sockfd, (struct sockaddr *) &cli_addr_buf, &cli_addrlen_buf);

	if (fd == -1)
		syscall_error("accept");

	MapBlock *block = map_get_block(map, (v3s32) {0, 0, 0}, false);
	assert(block);
	map_serialize_block(fd, block);
	close(fd);

	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);

	map_delete(map);
}
