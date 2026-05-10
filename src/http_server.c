#include "http_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#define BUFFER_SIZE 4096

static event_loop_t *g_loop = NULL;

static void client_callback(int fd, void *arg)
{
    char buffer[BUFFER_SIZE];

    int n = read(fd, buffer, sizeof(buffer) - 1);

    if(n <= 0) {
        printf("[INFO] client disconnected fd=%d\n", fd);
        
        close(fd);

        return;
    }

    buffer[n] = '\0';

    printf("====== REQUEST ======\n");
    printf("%s\n", buffer);

    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "hello mini-monitor\r\n";

    write(fd, response, strlen(response));

    close(fd);
}

static void accept_callback(int fd, void *arg)
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &len);

    if(client_fd < 0) {
        perror("accept");
        return;
    }

    printf("[INFO] new client fd=%d\n", client_fd);

    event_loop_add(
        g_loop,
        client_fd,
        client_callback,
        NULL
    );
}

int http_server_start(event_loop_t *loop, int port)
{
    g_loop = loop;

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(listen_fd < 0) {
        perror("socket");
        return -1;
    }

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