#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "array.h"

Array array_create(size_t membsiz)
{
	return (Array) {
		.membsiz = membsiz,
		.siz = 0,
		.cap = 0,
		.ptr = NULL,
		.cmp = NULL,
	};
}


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

void array_copy(Array *array, void **ptr, size_t *count)
{
	*count = array->siz;
	size_t size = array->siz * array->membsiz;
	*ptr = malloc(size);
	memcpy(*ptr, array->ptr, size);
}

ArraySearchResult array_search(Array *array, void *search)
{
	assert(array->cmp);
	size_t low, high, index;

	low = index = 0;
	high = array->siz;

	while (low < high) {
		index = low;

		size_t mid = (low + high) / 2;
		s8 state = array->cmp(search, (char *) array->ptr + mid * array->membsiz);

		if (state == 0)
			return (ArraySearchResult) {true, mid};
		else if (state > 0)
			high = mid;
		else
			low = mid + 1;
	}

	return (ArraySearchResult) {false, index};
}
