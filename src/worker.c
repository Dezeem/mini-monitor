#include "worker.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static task_queue_t g_queue;

static void process_task(task_t *task)
{
    char buffer[4096];

    int n = read(task->client_fd, buffer, sizeof(buffer) - 1);

    if(n <= 0) {
        close(task->client_fd);
        free(task);
        return;
    }

    buffer[n] = '\0';

    printf("====== REQUEST ======\n");
    printf("%s\n", buffer);

    /*
    * process
    */

    sleep(2);

    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 20\r\n"
        "\r\n"
        "hello mini-monitor\r\n";

    write(task->client_fd, response, strlen(response));

    close(task->client_fd);

    free(task);
}

static void *worker_thread(void *arg)
{
    while(1) {
        task_t *task = queue_pop(&g_queue);

        process_task(task);
    }

    return NULL;
}

int worker_pool_init(int num_threads)
{
    queue_init(&g_queue);

    for(int i  = 0;i < num_threads;i++) {
        pthread_t tid;

        pthread_create(&tid, NULL, worker_thread, NULL);

        pthread_detach(tid);
    }

    return 0;
}

void worker_submit(task_t *task)
{
    queue_push(&g_queue, task);
}