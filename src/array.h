#ifndef _ARRAY_H_
#define _ARRAY_H_

#define ARRAY_REALLOC_EXTRA 25

#include <stddef.h>
#include <stdbool.h>
#include "types.h"

typedef struct
{
	size_t membsiz;
	size_t siz, cap;
	void *ptr;
} Array;

Array array_create(size_t membsiz);
void array_insert(Array *array, void *elem, size_t idx);
void array_append(Array *array, void *elem);
void array_copy(Array *array, void **ptr, size_t *count);

#endif
