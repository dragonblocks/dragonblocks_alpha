#include <string.h>
#include <stdlib.h>
#include "array.h"

static void array_realloc(Array *array)
{
	if (array->siz > array->cap) {
		array->cap = array->siz + ARRAY_REALLOC_EXTRA;
		array->ptr = realloc(array->ptr, array->cap * sizeof(void *));
	}
}

static void array_alloc(Array *array, size_t siz)
{
	array->siz += siz;
	array_realloc(array);
}

void array_insert(Array *array, void *elem, size_t idx)
{
	size_t oldsiz = array->siz;
	array_alloc(array, 1);

	void **iptr = array->ptr + idx;
	memmove(iptr + 1, iptr, (oldsiz - idx) * sizeof(void *));
	*iptr = elem;
}

void array_append(Array *array, void *elem)
{
	size_t oldsiz = array->siz;
	array_alloc(array, 1);

	array->ptr[oldsiz] = elem;
}

Array array_create()
{
	return (Array) {0, 0, NULL};
}
