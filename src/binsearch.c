#include "binsearch.h"

BinsearchResult binsearch(void *search, void **array, size_t size, BinsearchComparator cmp)
{
	size_t min, max, index;

	min = index = 0;
	max = size;

	while (min < max) {
		index = min;

		size_t mid = (max + min) / 2;
		s8 state = cmp(search, array[mid]);

		if (state == 0)
			return (BinsearchResult) {true, mid};
		else if (state > 0)
			max = mid;
		else
			min = mid;
	}

	return (BinsearchResult) {false, index};
}
