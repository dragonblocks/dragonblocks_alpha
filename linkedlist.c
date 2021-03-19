#include <stdlib.h>
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
