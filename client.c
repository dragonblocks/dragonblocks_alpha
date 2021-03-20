#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "map.h"
#include "util.h"

int main(int argc, char **argv)
{
	program_name = argv[0];

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd == -1)
		syscall_error("socket");

	if (argc <= 1)
		internal_error("missing address");

	struct in_addr addr_buf;

	if (inet_aton(argv[1], &addr_buf) == 0)
		internal_error("invalid address");

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = get_port_from_args(argc, argv, 2),
		.sin_addr = addr_buf,
	};

	if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		syscall_error("connect");

	Map *map = map_create(NULL);

	MapBlock *block = map_deserialize_block(sockfd);
	if (block)
		map_create_block(map, (v3s32) {0, 0, 0}, block);
	else
		internal_error("invalid block recieved");

	MapNode node = map_get_node(map, (v3s32) {0, 0, 0});
	printf("%d\n", node.type);

	close(sockfd);

	map_delete(map);
}
