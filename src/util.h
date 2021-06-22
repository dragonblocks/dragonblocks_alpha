#ifndef _UTIL_H_
#define _UTIL_H_

#include <arpa/inet.h>
#include <linmath.h/linmath.h>
#include "types.h"

#define ever (;;)
#define INBRACES(str) str ? "(" : "", str ? str : "", str ? ")" : ""
#define CMPBOUNDS(x) x == 0 ? 0 : x > 0 ? 1 : -1

extern const char *program_name;

void syscall_error(const char *err);
void internal_error(const char *err);
char *read_string(int fd, size_t bufsiz);
char *address_string(struct sockaddr_in6 *addr);
v3f html_to_v3f(const char *html);

#endif
