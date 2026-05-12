#define _GNU_SOURCE
#include "metrics.h"

static metrics_t g_metrics;

int metrics_init()
{
    atomic_init(
        &g_metrics.total_requests, 
        0
    );

    atomic_init(
        &g_metrics.active_connections, 
        0
    );

    return 0;
}

void metrics_inc_requests()
{
    atomic_fetch_add(
        &g_metrics.total_requests, 
        1
    );
}

void metrics_inc_connections()
{
    atomic_fetch_add(
        &g_metrics.active_connections, 
        1
    );
}

void metrics_dec_connections()
{
    atomic_fetch_sub(
        &g_metrics.active_connections,
        1
    );
}

void metrics_snapshot(metrics_t *out)
{
    out->active_connections = atomic_load(&g_metrics.active_connections);

    out->total_requests = atomic_load(&g_metrics.total_requests);
}
