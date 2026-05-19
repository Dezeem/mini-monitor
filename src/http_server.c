#include "http_server.h"
#include "worker.h"
#include "connection.h"
#include "metrics.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <arpa/inet.h>

#define BUFFER_SIZE 4096

static event_loop_t *g_loop = NULL;

static void client_callback(int fd, void *arg)
{
    connection_t *conn = (connection_t *)arg;

    while(1) {
        int n = read(fd, conn->read_buf, BUFF_SIZE - conn->read_len -1);

        if(n > 0) {
            conn->read_len += n;

            conn->read_buf[conn->read_len] = '\0';
        }
        else if(n == 0) {
            printf("[INFO] client closed\n");

            metrics_dec_connections();

            close(fd);

            return;
        }
        else {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("read");

            metrics_dec_connections();

            close(fd);

            free(conn);

            return;
        }
    }

    /*
    * Simple HTTP Termination Check.
    */

    if(strstr(conn->read_buf, "\r\n\r\n")) {
        if(strstr(conn->read_buf, "GET /ping HTTP/1.1")) {
            handle_ping(fd);

            metrics_dec_connections();

            close(fd);

            free(conn);

            return;
        } else {
            task_t *task = malloc(sizeof(task_t));

            task->client_fd = fd;

            strncpy(
                task->request,
                conn->read_buf,
                sizeof(task->request) - 1
            );

            worker_submit(task);
        }
    }
}

static void accept_callback(int fd, void *arg)
{
    while(1) {

        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        int client_fd = accept(fd, (struct sockaddr *)&client_addr, &len);

        if(client_fd < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }

            perror("accept");
            return;
        }

        set_nonblocking(client_fd);

        printf("[INFO] new client fd=%d\n", client_fd);

        connection_t *conn = malloc(sizeof(connection_t));
        memset(conn, 0, sizeof(*conn));
        conn->fd = client_fd;

        metrics_inc_connections();

        event_loop_add(
            g_loop,
            client_fd,
            client_callback,
            conn
        );
    }
}

int http_server_start(event_loop_t *loop, int port)
{
    g_loop = loop;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd < 0) {
        perror("socket");
        return -1;
    }

    set_nonblocking(listen_fd);

    int opt = 1;

    setsockopt(
        listen_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

    struct sockaddr_in addr;
    
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if(bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    if(listen(listen_fd, 128)) {
        perror("listen");
        return -1;
    }

    printf("[INFO] HTTP server listen on %d\n", port);

    event_loop_add(
        loop,
        listen_fd,
        accept_callback,
        NULL
    );

    return 0;
}

int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if(flags < 0) {
        perror("fcntl F_GETFL");
        return -1;
    }

    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        return -1;
    }

    return 0;
}