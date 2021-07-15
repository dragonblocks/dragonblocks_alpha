#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <pthread.h>
#include "list.h"

typedef struct
{
	List list;
	pthread_mutex_t mtx;
} Queue;

Queue *queue_create();
void queue_delete(Queue *queue);
void queue_enqueue(Queue *queue, void *elem);
void *queue_dequeue(Queue *queue);
void *queue_dequeue_callback(Queue *queue, void (*callback)(void *elem));

#endif
