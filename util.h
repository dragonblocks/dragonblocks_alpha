#ifndef _UTIL_H_
#define _UTIL_H_

#include "types.h"

extern const char *program_name;

void syscall_error(const char *err);
void internal_error(const char *err);
u16 get_port_from_args(int argc, char **argv, int index);

#endif
