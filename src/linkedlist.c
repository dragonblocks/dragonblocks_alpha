#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"

LinkedList linked_list_create()
{
	return (LinkedList) {NULL};
}

void linked_list_clear(LinkedList *list)
{
	for (LinkedListPair *pair = list->first; pair != NULL;) {
		LinkedListPair *next = pair->next;
		free(pair);
		pair = next;
	}
	list->first = NULL;
}

static LinkedListPair *make_pair(const char *key, void *value)
{
	LinkedListPair *pair = malloc(sizeof(LinkedListPair));
	pair->key = key;
	pair->value = value;
	pair->next = NULL;
	return pair;
}

bool linked_list_put(LinkedList *list, const char *key, void *value)
{
	LinkedListPair **pairptr;
	for (pairptr = &list->first; *pairptr != NULL; pairptr = &(*pairptr)->next) {
		if (strcmp((*pairptr)->key, key) == 0)
			return false;
	}
	*pairptr = make_pair(key, value);
	return true;
}

void linked_list_set(LinkedList *list, const char *key, void *value)
{
	LinkedListPair **pairptr;
	for (pairptr = &list->first; *pairptr != NULL; pairptr = &(*pairptr)->next) {
		if (strcmp((*pairptr)->key, key) == 0)
			break;
	}
	*pairptr = make_pair(key, value);
}

void linked_list_delete(LinkedList *list, const char *key)
{
	for (LinkedListPair **pairptr = &list->first; *pairptr != NULL; pairptr = &(*pairptr)->next) {
		if (strcmp((*pairptr)->key, key) == 0) {
			LinkedListPair *pair = *pairptr;
			*pairptr = pair->next;
			free(pair);
			return;
		}
	}
}

void *linked_list_get(LinkedList *list, const char *key)
{
	for (LinkedListPair *pair = list->first; pair != NULL; pair = pair->next)
		if (strcmp(pair->key, key) == 0)
			return pair->value;
	return NULL;
}
