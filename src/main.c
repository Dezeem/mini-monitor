#include "event_loop.h"
#include "http_server.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/timerfd.h>

static void timer_callback(int fd, void *arg)
{
    uint64_t exp;

    read(fd, &exp, sizeof(exp));

    printf("[INFO] timer triggered %lu\n", exp);
}

int main()
{
    event_loop_t loop;

    if(event_loop_init(&loop) < 0) {
        return -1;
    }

    printf("[INFO] event loop start\n");

    /*
    * timerfd
    */

    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);

    struct itimerspec ts;

    memset(&ts, 0, sizeof(ts));

    ts.it_value.tv_sec = 1;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 1;
    ts.it_interval.tv_nsec = 0;

    timerfd_settime(tfd, 0, &ts, NULL);

    event_loop_add(&loop, tfd, timer_callback, NULL);

    /*
    * http_server
    */

    http_server_start(
        &loop,
        8080
    );

    event_loop_run(&loop);

    return 0;
}