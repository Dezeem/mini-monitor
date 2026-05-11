#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include <pthread.h>

typedef struct {

    uint64_t total_requests;

    uint64_t active_connections;

} metrics_t;

int metrics_init();

void metrics_inc_requests();

void metrics_inc_connections();

void metrics_dec_connections();

void metrics_snapshot(metrics_t *out);

#endif