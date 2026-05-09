#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <stdint.h>
typedef void (*event_callback_t)(int fd, void *arg);

typedef struct {
    int fd;
    event_callback_t call_back;
    void *arg;
} event_handler_t;

typedef struct {
    int epfd;
    event_handler_t handlers[1024];
} event_loop_t;

int event_loop_init(event_loop_t *loop);

int event_loop_add(
    event_loop_t *loop,
    int fd,
    event_callback_t cb,
    void *arg
);

void event_loop_run(event_loop_t *loop);


#endif