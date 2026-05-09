#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/epoll.h>

#include "event_loop.h"

#define MAX_EVENTS 64

int event_loop_init(event_loop_t *loop)
{
    memset(loop, 0, sizeof(*loop));

    loop->epfd = epoll_create1(0);

    if(loop->epfd < 0) {
        perror("epoll_creat1");
        return -1;
    }

    return 0;
}

int event_loop_add(event_loop_t *loop, int fd, event_callback_t cb, void *arg)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    if(epoll_ctl(loop->epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("epoll_ctl");
        return -1;
    }

    loop->handlers[fd].fd = fd;
    loop->handlers[fd].call_back = cb;
    loop->handlers[fd].arg = arg;

    return 0;
}

void event_loop_run(event_loop_t *loop)
{
    struct epoll_event events[MAX_EVENTS];

    while(1) {
        int n = epoll_wait(loop->epfd, events, MAX_EVENTS, -1);
        if(n < 0) {
            perror("epoll_wait");
            continue;
        }

        for(int i = 0;i < n;i++) {
            int fd = events[i].data.fd;

            event_handler_t *handler = &loop->handlers[fd];

            if(handler->call_back) {
                handler->call_back(fd, handler->arg);
            }
        }
    }
}
