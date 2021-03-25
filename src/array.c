#include <string.h>
#include <stdlib.h>
#include "array.h"

static void array_realloc(Array *array)
{
	if (array->siz > array->cap) {
		array->cap = array->siz + ARRAY_REALLOC_EXTRA;
		array->ptr = realloc(array->ptr, array->cap * array->membsiz);
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

	char *iptr = (char *) array->ptr + idx * array->membsiz;
	memmove(iptr + array->membsiz, iptr, (oldsiz - idx) * array->membsiz);
	memcpy(iptr, elem, array->membsiz);
}

void array_append(Array *array, void *elem)
{
	size_t oldsiz = array->siz;
	array_alloc(array, 1);

	memcpy((char *) array->ptr + oldsiz * array->membsiz, elem, array->membsiz);
}

Array array_create(size_t membsiz)
{
	return (Array) {
		.membsiz = membsiz,
		.siz = 0,
		.cap = 0,
		.ptr = NULL,
	};
}
