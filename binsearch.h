#ifndef _BINSEARCH_H_
#define _BINSEARCH_H_

#include <stddef.h>
#include <stdbool.h>
#include "types.h"

typedef s8 (*BinsearchComparator)(void *search, void *element);

typedef struct {
	bool success;
	size_t index;
} BinsearchResult;

BinsearchResult binsearch(void *search, void **array, size_t size, BinsearchComparator cmp);

#endif
