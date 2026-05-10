#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "event_loop.h"

int http_server_start(
    event_loop_t *loop,
    int port
);

#endif