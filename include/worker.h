#ifndef WORKER_H
#define WORKER_H

typedef struct 
{
    int client_fd;

    char request[4096];
    
} task_t;

int worker_pool_init(int num_threads);

void worker_submit(task_t *task);

void handle_ping(int client_fd);

#endif