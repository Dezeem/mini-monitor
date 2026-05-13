#include "worker.h"
#include "queue.h"
#include "metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static task_queue_t g_queue;

static void process_task(task_t *task)
{
    metrics_inc_requests();

    char response_buf[2048];
    const char *response_ptr = NULL;
    size_t response_len = 0;

    if(strstr(task->request, "/metrics")) {
        metrics_t snapshot;
        metrics_snapshot(&snapshot);

        char body[1024];
        int body_len = snprintf(body, sizeof(body),
                                "requests_total %lu\n"
                                "active_connections %lu\n",
                                snapshot.total_requests,
                                snapshot.active_connections);

        int len = snprintf(response_buf, sizeof(response_buf),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           body_len, body);

        if(len > 0 && len < (int)sizeof(response_buf)) {
            response_ptr = response_buf;
            response_len = len;
        } else {
            response_ptr = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
            response_len = strlen(response_ptr);
        }
    } else {
        response_ptr = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: 20\r\n"
                       "Connection: close\r\n"
                       "\r\n"
                       "hello mini-monitor\r\n";
        response_len = strlen(response_ptr);
    }

    write(task->client_fd, response_ptr, response_len);

    metrics_dec_connections();

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