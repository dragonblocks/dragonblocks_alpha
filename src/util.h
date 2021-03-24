#ifndef _UTIL_H_
#define _UTIL_H_

#include <arpa/inet.h>
#include "types.h"

#define ever (;;)
#define INBRACES(str) str ? "(" : "", str ? str : "", str ? ")" : ""

extern const char *program_name;

void syscall_error(const char *err);
void internal_error(const char *err);
char *read_string(int fd, size_t bufsiz);
char *address_string(struct sockaddr_in6 *addr);

#endif
