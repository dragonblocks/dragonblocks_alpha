#ifndef _ARRAY_H_
#define _ARRAY_H_

#define ARRAY_REALLOC_EXTRA 25

#include <stddef.h>

typedef struct
{
	size_t siz, cap;
	void **ptr;
} Array;

void array_insert(Array *array, void *elem, size_t idx);
void array_append(Array *array, void *elem);
Array array_create();

#endif
