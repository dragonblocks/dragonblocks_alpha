#ifndef _LINKEDLIST_H_
#define _LINKEDLIST_H_

#define ITERATE_LINKEDLIST(list, pair) for (LinkedListPair *pair = list->first; pair != NULL; pair = pair->next)

typedef struct LinkedListPair
{
	struct LinkedListPair *next;
	const char *key;
	const char *value;
} LinkedListPair;

typedef struct
{
	LinkedListPair *first;
} LinkedList;

LinkedList linked_list_create();
void linked_list_clear(LinkedList *list);

void linked_list_put(LinkedList *list, const char *key, const char *value); // ToDo
void linked_list_get(LinkedList *list, const char *key); // ToDo
void linked_list_delete(LinkedList *list, const char *key); // ToDo

void linked_list_serialize(int fd); // ToDo
void linked_list_deserialize(int fd, LinkedList *); // ToDo

#endif
