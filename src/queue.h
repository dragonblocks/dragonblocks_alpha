#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <pthread.h>
#include "list.h"

typedef struct
{
	List list;
	pthread_mutex_t mtx;
} Queue;

Queue *create_queue();
void delete_queue(Queue *queue);
void enqueue(Queue *queue, void *elem);
void *dequeue(Queue *queue);
void *dequeue_callback(Queue *queue, void (*callback)(void *elem));

#endif
