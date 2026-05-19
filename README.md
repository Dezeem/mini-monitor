# Mini-Monitor

A lightweight high-concurrency monitoring service built on Linux `epoll` ET mode.

The project started from a simple blocking server and gradually evolved into an event-driven architecture with:

* non-blocking socket IO
* epoll Edge Triggered mode
* Reactor event model
* thread pool
* connection state management
* metrics subsystem
* atomic counter optimization
* fast path bypass worker optimization

The focus of this project is not implementing a full-featured HTTP server, but understanding:

* Linux high-performance IO models
* Reactor architecture
* latency / throughput tradeoffs
* synchronization overhead
* hot path optimization
* performance bottleneck analysis

---

# Table of Contents

* [Architecture](#architecture)
* [Core Components](#core-components)
* [Event Model](#event-model)
* [Thread Pool Design](#thread-pool-design)
* [Metrics Subsystem](#metrics-subsystem)
* [Performance Optimization](#performance-optimization)
* [Benchmark](#benchmark)
* [Build & Run](#build--run)
* [Project Evolution](#project-evolution)
* [Key Takeaways](#key-takeaways)
* [Future Improvements](#future-improvements)

---

# Architecture

```text
                 +------------------+
                 |   epoll thread   |
                 +--------+---------+
                          |
                    accept / read
                          |
             +------------+------------+
             |                         |
        fast path                 slow path
   (/ping, /metrics)            worker pool
             |                         |
          response               business logic
             |                         |
           close                    response
```

The server uses a single Reactor thread to handle socket events.

Lightweight requests such as:

* `/ping`
* `/metrics`

are processed directly inside the epoll thread to avoid unnecessary scheduling overhead.

Slow or simulated business logic is dispatched to the worker pool.

---

# Core Components

## event_loop

Responsible for:

* `epoll_create`
* `epoll_wait`
* fd event dispatch

Implements a callback-based Reactor model.

---

## http_server

Responsible for:

* socket creation
* non-blocking configuration
* accept loop
* HTTP request parsing

---

## worker

Implements a producer-consumer model.

Used to decouple:

* IO processing
* business logic

from the Reactor thread.

---

## metrics

Provides runtime statistics including:

* total requests
* active connections

The initial implementation used `pthread_rwlock`.

Later versions switched to atomic counters to reduce synchronization overhead.

---

# Event Model

The server uses `epoll` in Edge Triggered (ET) mode.

Under ET mode, socket events are only triggered when the socket state changes.

Therefore:

* sockets must be non-blocking
* reads must continue until `EAGAIN`

Otherwise unread data may remain in the socket buffer and no further events will be delivered.

Example:

```c
while (1) {
    n = read(fd, buf, sizeof(buf));

    if (n > 0) {
        ...
    }
    else if (n == -1 && errno == EAGAIN) {
        break;
    }
}
```

Blocking operations inside the Reactor thread will stall the entire event loop.

This affects:

* accepting new connections
* timer events
* processing subsequent socket events

To avoid this, slow tasks are dispatched to worker threads.

---

# Thread Pool Design

The initial implementation executed all business logic directly inside the Reactor thread.

While simple, this design caused the event loop to stall whenever a request became slow.

A thread pool was later introduced to separate:

* event handling
* business processing

The architecture follows a producer-consumer model:

```text
                 +------------------+
                 |   epoll thread   |
                 +--------+---------+
                          |
                     task queue
                          |
              +-----------+-----------+
              |                       |
         worker 1                 worker 2
              |                       |
         business logic         business logic
```

## Tradeoff

The thread pool improves throughput and prevents slow requests from blocking the Reactor.

However, benchmarking showed that it also introduces additional overhead:

* mutex contention
* condition variable wakeup
* context switch
* scheduler latency
* cache miss

For lightweight requests, these costs may exceed the actual request processing time.

---

# Metrics Subsystem

The metrics subsystem exposes runtime statistics through `/metrics`.

Tracked metrics include:

* total requests
* active connections

The initial implementation used `pthread_rwlock` to protect shared state.

Since the metrics workload primarily consists of simple counters, later versions replaced rwlocks with atomic operations:

* reduced lock contention
* lower synchronization overhead
* lower latency under concurrency

Metrics are read using a snapshot model to avoid long critical sections.

---

# Performance Optimization

## Logging Overhead

Benchmarking revealed that synchronous `printf` logging significantly degraded performance.

The overhead mainly came from:

* syscall cost
* stdout lock contention
* terminal IO latency
* formatting overhead

Removing logging from the request hot path produced a major improvement:

| Version        | QPS  | Avg Latency |
| -------------- | ---- | ----------- |
| baseline       | ~8k  | ~20ms       |
| remove logging | ~17k | ~3ms        |

---

## Fast Path Optimization

Further profiling showed that for lightweight requests such as:

* `/ping`
* `/metrics`

the worker queue itself became a bottleneck.

To reduce scheduling overhead, a fast path was added:

* lightweight requests are processed directly in the epoll thread
* slow requests continue to use the worker pool

This avoids:

* queue push/pop
* condition wakeup
* unnecessary context switch

Updated architecture:

```text
                 +------------------+
                 |   epoll thread   |
                 +--------+---------+
                          |
                +---------+---------+
                |                   |
           fast path           slow path
        (/ping,/metrics)       worker pool
                |                   |
             response         business logic
```

Benchmark result:

| Version                 | QPS  | Avg Latency |
| ----------------------- | ---- | ----------- |
| baseline                | ~8k  | ~20ms       |
| remove logging          | ~17k | ~3ms        |
| fast path bypass worker | ~19k | ~2ms        |

As the system evolved, performance bottlenecks gradually shifted:

```text
logging
→ scheduler
→ synchronization
→ syscall / kernel path
```

---

# Benchmark

Environment:

* Linux
* gcc
* epoll ET mode
* 4 worker threads

Benchmark tool:

```bash
wrk -t4 -c100 -d10s \
http://127.0.0.1:8080/ping
```

Result:

```text
Running 10s test @ http://127.0.0.1:8080/ping
  4 threads and 100 connections

Latency     2.05ms
Requests/sec: 19014.61
```

---

# Build & Run

## Requirements

* Linux
* gcc
* make

---

## Build

```bash
make
```

---

## Run

```bash
./mini-monitor
```

Default address:

```text
127.0.0.1:8080
```

---

## Test

### ping

```bash
curl http://127.0.0.1:8080/ping
```

### metrics

```bash
curl http://127.0.0.1:8080/metrics
```

---

# Project Evolution

The project evolved incrementally instead of being designed upfront:

1. blocking server
2. epoll Reactor
3. ET + nonblocking socket
4. thread pool
5. connection state management
6. metrics subsystem
7. atomic optimization
8. remove hot path logging
9. fast path bypass worker

The main goal was not feature accumulation, but understanding:

* system bottlenecks
* latency sources
* synchronization overhead
* architecture tradeoffs
* real-world optimization workflow

---

# Key Takeaways

## IO is not the only bottleneck

Performance bottlenecks may also come from:

* logging
* synchronization
* scheduler latency
* context switch
* cache behavior

---

## Thread pools improve throughput but may increase latency

Thread pools prevent slow tasks from blocking the Reactor thread.

At the same time, they introduce:

* scheduling overhead
* synchronization cost
* additional context switches

For lightweight workloads, these costs become significant.

---

## Hot path design matters

Operations inside the request hot path must remain minimal.

Even simple operations such as:

* `printf`
* unnecessary synchronization
* blocking syscalls

can significantly impact latency.

---

## ET mode requires non-blocking IO

Under ET mode, sockets must be drained until `EAGAIN`.

Failing to do so may leave unread data in the socket buffer and prevent future event notifications.

---

# Future Improvements

Possible future optimizations include:

* per-thread queue
* object pool
* async logging
* zero-copy response
* io_uring support
* keep-alive connection support
* HTTP parser optimization

The current focus of the project remains:

* Reactor architecture
* Linux high-concurrency IO
* latency / throughput tradeoffs
* system performance analysis