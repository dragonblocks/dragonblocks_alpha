#include <stdlib.h>
#include "queue.h"

Queue *create_queue()
{
	Queue *queue = malloc(sizeof(Queue));
	queue->list = list_create(NULL);
	pthread_mutex_init(&queue->mtx, NULL);
	return queue;
}

void delete_queue(Queue *queue)
{
	pthread_mutex_destroy(&queue->mtx);
	list_clear(&queue->list);
	free(queue);
}

void enqueue(Queue *queue, void *elem)
{
	pthread_mutex_lock(&queue->mtx);
	list_put(&queue->list, elem, NULL);
	pthread_mutex_unlock(&queue->mtx);
}

void *dequeue(Queue *queue)
{
	return dequeue_callback(queue, NULL);
}

void *dequeue_callback(Queue *queue, void (*callback)(void *elem))
{
	pthread_mutex_lock(&queue->mtx);
	void *elem = NULL;
	ListPair **lptr = &queue->list.first;
	if (*lptr) {
		elem = (*lptr)->key;
		ListPair *next = (*lptr)->next;
		free(*lptr);
		*lptr = next;

		if (callback)
			callback(elem);
	}
	pthread_mutex_unlock(&queue->mtx);
	return elem;
}
