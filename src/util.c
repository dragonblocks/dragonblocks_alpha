#include <stdio.h>
#include <stdlib.h>
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

char *address_string(struct sockaddr_in6 *addr)
{
	char address[INET6_ADDRSTRLEN] = {0};
	char port[6] = {0};

	if (inet_ntop(addr->sin6_family, &addr->sin6_addr, address, INET6_ADDRSTRLEN) == NULL)
		perror("inet_ntop");
	sprintf(port, "%d", ntohs(addr->sin6_port));

	char *result = malloc(strlen(address) + 1 + strlen(port) + 1);
	sprintf(result, "%s:%s", address, port);
	return result;
}

v3f html_to_v3f(const char *html)
{
	unsigned int r, g, b;
	sscanf(html, "#%2x%2x%2x", &r, &g, &b);
	return (v3f) {(f32) r / 255.0f, (f32) g / 255.0f, (f32) b / 255.0f};
}
