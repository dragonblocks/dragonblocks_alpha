#include <stdlib.h>
#include <string.h>
#include "list.h"

bool list_compare_default(void *v1, void *v2)
{
	return v1 == v2;
}

bool list_compare_string(void *v1, void *v2)
{
	return strcmp(v1, v2) == 0;
}

List list_create(ListComparator cmp)
{
	return (List) {
		.cmp = cmp ? cmp : list_compare_default,
		.first = NULL,
	};
}

void list_clear(List *list)
{
	for (ListPair *pair = list->first; pair != NULL;) {
		ListPair *next = pair->next;
		free(pair);
		pair = next;
	}
	list->first = NULL;
}

static ListPair *make_pair(void *key, void *value)
{
	ListPair *pair = malloc(sizeof(ListPair));
	pair->key = key;
	pair->value = value;
	pair->next = NULL;
	return pair;
}

bool list_put(List *list, void *key, void *value)
{
	ListPair **pairptr;
	for (pairptr = &list->first; *pairptr != NULL; pairptr = &(*pairptr)->next) {
		if (list->cmp((*pairptr)->key, key))
			return false;
	}
	*pairptr = make_pair(key, value);
	return true;
}

void list_set(List *list, void *key, void *value)
{
	ListPair **pairptr;
	for (pairptr = &list->first; *pairptr != NULL; pairptr = &(*pairptr)->next) {
		if (strcmp((*pairptr)->key, key) == 0)
			break;
	}
	*pairptr = make_pair(key, value);
}

void *list_delete(List *list, void *key)
{
	for (ListPair **pairptr = &list->first; *pairptr != NULL; pairptr = &(*pairptr)->next) {
		if (list->cmp((*pairptr)->key, key)) {
			ListPair *pair = *pairptr;
			void *value = (*pairptr)->value;
			*pairptr = pair->next;
			free(pair);
			return value;
		}
	}
	return NULL;
}

void *list_get(List *list, void *key)
{
	for (ListPair *pair = list->first; pair != NULL; pair = pair->next)
		if (list->cmp(pair->key, key))
			return pair->value;
	return NULL;
}
