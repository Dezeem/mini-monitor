#ifndef WORKER_H
#define WORKER_H

typedef struct 
{
    int client_fd;
} task_t;

int worker_pool_init(int num_threads);

void worker_submit(task_t *task);

#endif