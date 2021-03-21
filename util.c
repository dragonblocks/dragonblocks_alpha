#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "util.h"

const char *program_name;

void syscall_error(const char *err)
{
	perror(err);
	exit(EXIT_FAILURE);
}

void internal_error(const char *err)
{
	fprintf(stderr, "%s: %s\n", program_name, err);
	exit(EXIT_FAILURE);
}


unsigned short get_port_from_args(int argc, char **argv, int index)
{
	if (argc <= index)
		internal_error("missing port");

	unsigned int port = atoi(argv[index]);

	if (port == 0 || port > USHRT_MAX)
		internal_error("invalid port");

	return htons(port);
}

char *read_string(int fd, size_t bufsiz)
{
	char buf[bufsiz + 1];
	buf[bufsiz] = 0;
	for (size_t i = 0;; i++) {
		char c;
		if (read(fd, &c, 1) == -1) {
			perror("read");
			return NULL;
		}
		if (i < bufsiz)
			buf[i] = c;
		if (c == EOF || c == 0)
			break;
	}
	return strdup(buf);
}
