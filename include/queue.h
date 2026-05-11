#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>

#define QUEUE_SIZE 1024

typedef struct
{
    void *items[QUEUE_SIZE];

    int head;
    int tail;
    int count;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

} task_queue_t;

int queue_init(task_queue_t *q);

int queue_push(task_queue_t *q, void *item);

void *queue_pop(task_queue_t *q);

#endif
