#define _GNU_SOURCE
#include "metrics.h"

static metrics_t g_metrics;

static pthread_rwlock_t g_lock;

int metrics_init()
{
    pthread_rwlock_init(&g_lock, NULL);

    return 0;
}

void metrics_inc_requests()
{
    pthread_rwlock_wrlock(&g_lock);

    g_metrics.total_requests++;

    pthread_rwlock_unlock(&g_lock);
}

void metrics_inc_connections()
{
    pthread_rwlock_wrlock(&g_lock);

    g_metrics.active_connections++;

    pthread_rwlock_unlock(&g_lock);
}

void metrics_dec_connections()
{
    pthread_rwlock_wrlock(&g_lock);

    g_metrics.active_connections--;

    pthread_rwlock_unlock(&g_lock);
}

void metrics_snapshot(metrics_t *out)
{
    pthread_rwlock_rdlock(&g_lock);

    *out = g_metrics;

    pthread_rwlock_unlock(&g_lock);
}
