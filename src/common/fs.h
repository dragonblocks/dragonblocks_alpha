#ifndef _FS_H_
#define _FS_H_

#include <stdbool.h>

char *read_file(const char *path);
bool directory_exists(const char *path);

#endif
