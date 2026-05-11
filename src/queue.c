#include "queue.h"

int queue_init(task_queue_t *q)
{
    q->head = 0;
    q->tail = 0;
    q->count = 0;

    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);

    return 0;
}

int queue_push(task_queue_t *q, void *item)
{
    pthread_mutex_lock(&q->mutex);

    if(q->count >= QUEUE_SIZE) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }

    q->items[q->tail] = item;

    q->tail = (q->tail + 1) % QUEUE_SIZE;

    q->count++;

    pthread_cond_signal(&q->cond);

    pthread_mutex_unlock(&q->mutex);

    return 0;
}

void *queue_pop(task_queue_t *q)
{
    pthread_mutex_lock(&q->mutex);

    while(q->count == 0) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    void *item = q->items[q->head];

    q->head = (q->head + 1) % QUEUE_SIZE;

    q->count--;

    pthread_mutex_unlock(&q->mutex);

    return item;
}

