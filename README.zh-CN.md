# Mini-Monitor

一个基于 Linux `epoll` ET 模式实现的轻量高并发监控服务。

项目从最初的阻塞式服务器开始，逐步演进为包含以下特性的事件驱动架构：

* non-blocking socket IO
* epoll Edge Triggered 模式
* Reactor 事件模型
* thread pool
* connection state 管理
* metrics subsystem
* atomic counter 优化
* fast path bypass worker 优化

项目重点并不在于实现完整 Web Server，而是围绕以下方向进行实践：

* Linux 高性能 IO 模型
* Reactor 架构
* latency / throughput tradeoff
* 多线程同步开销
* hot path 优化
* 系统性能分析与瓶颈定位

---

# 目录

* [系统架构](#系统架构)
* [核心模块](#核心模块)
* [事件模型](#事件模型)
* [线程池设计](#线程池设计)
* [Metrics 子系统](#metrics-子系统)
* [性能优化](#性能优化)
* [压测结果](#压测结果)
* [构建与运行](#构建与运行)
* [项目演进过程](#项目演进过程)
* [关键收获](#关键收获)
* [未来优化方向](#未来优化方向)

---

# 系统架构

```text id="0x1j9m"
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

系统采用单 Reactor 线程负责 socket 事件处理。

对于：

* `/ping`
* `/metrics`

等轻量请求，

直接在 epoll thread 内完成处理，避免额外的调度与线程切换开销。

耗时业务则交由 worker pool 处理。

---

# 核心模块

## event_loop

负责：

* `epoll_create`
* `epoll_wait`
* fd event dispatch

实现基于 callback 的 Reactor 模型。

---

## http_server

负责：

* socket 创建
* non-blocking 设置
* accept loop
* HTTP request parsing

---

## worker

实现 producer-consumer 模型。

用于解耦：

* IO processing
* business logic

避免耗时任务阻塞 Reactor。

---

## metrics

提供运行时统计信息，包括：

* total requests
* active connections

初始版本使用 `pthread_rwlock` 保护共享状态。

后续版本改为 atomic counter，降低同步开销。

---

# 事件模型

系统采用 `epoll` Edge Triggered（ET）模式。

ET 模式下：

socket 状态变化仅触发一次通知。

因此：

* socket 必须使用 non-blocking
* 必须持续读取直到返回 `EAGAIN`

否则可能导致 socket buffer 中仍有数据，但后续不再收到事件通知。

示例：

```c id="6u2hqp"
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

如果在 Reactor 线程中执行阻塞操作：

* 新连接无法 accept
* timer event 无法处理
* 后续 socket event 无法及时响应

因此慢任务需要交由 worker thread 处理。

---

# 线程池设计

初始版本中：

所有业务逻辑均直接运行在 Reactor 线程内。

虽然实现简单，但一旦某个请求变慢，整个 event loop 都会被阻塞。

后续引入 thread pool，将：

* event handling
* business processing

进行解耦。

线程模型如下：

```text id="0lv74m"
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

---

## Tradeoff

thread pool 能够避免慢业务阻塞 Reactor，

但同时也会引入额外开销：

* mutex contention
* condition variable wakeup
* context switch
* scheduler latency
* cache miss

对于轻量请求而言，

这些调度成本可能已经超过请求本身的处理成本。

---

# Metrics 子系统

系统通过 `/metrics` 暴露运行时指标。

当前支持：

* total requests
* active connections

初始实现使用 `pthread_rwlock` 保护共享状态。

由于 metrics 场景主要为简单计数，

后续改用 atomic operation：

* 降低 lock contention
* 减少 synchronization overhead
* 提升高并发下的 latency 表现

metrics 读取采用 snapshot 模型，避免长时间持锁。

---

# 性能优化

## Logging 开销分析

压测过程中发现：

同步 `printf` logging 会显著降低系统性能。

主要开销来自：

* syscall
* stdout lock contention
* terminal IO latency
* formatting overhead

移除 hot path logging 后：

| Version        | QPS  | Avg Latency |
| -------------- | ---- | ----------- |
| baseline       | ~8k  | ~20ms       |
| remove logging | ~17k | ~3ms        |

---

## Fast Path 优化

进一步压测发现：

对于：

* `/ping`
* `/metrics`

等轻量请求，

worker queue 本身已经成为主要瓶颈。

因此增加：

fast path bypass worker

架构：

轻量请求直接在 epoll thread 内完成处理，

避免：

* queue push/pop
* condition wakeup
* unnecessary context switch

优化后架构：

```text id="jcmc0l"
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

优化结果：

| Version                 | QPS  | Avg Latency |
| ----------------------- | ---- | ----------- |
| baseline                | ~8k  | ~20ms       |
| remove logging          | ~17k | ~3ms        |
| fast path bypass worker | ~19k | ~2ms        |

随着系统演进，

性能瓶颈也逐渐发生迁移：

```text id="x3l0up"
logging
→ scheduler
→ synchronization
→ syscall / kernel path
```

---

# 压测结果

测试环境：

* Linux
* gcc
* epoll ET mode
* 4 worker threads

压测工具：

```bash id="qj5sm9"
wrk -t4 -c100 -d10s \
http://127.0.0.1:8080/ping
```

结果：

```text id="s7n7s9"
Running 10s test @ http://127.0.0.1:8080/ping
  4 threads and 100 connections

Latency     2.05ms
Requests/sec: 19014.61
```

---

# 构建与运行

## 环境要求

* Linux
* gcc
* make

---

## 编译

```bash id="w69gcr"
make
```

---

## 运行

```bash id="obd4zm"
./mini-monitor
```

默认监听地址：

```text id="35g5nl"
127.0.0.1:8080
```

---

## 测试

### ping

```bash id="crbm3v"
curl http://127.0.0.1:8080/ping
```

### metrics

```bash id="8z1nml"
curl http://127.0.0.1:8080/metrics
```

---

# 项目演进过程

项目并非一次性完成，

而是随着性能分析逐步演进：

1. blocking server
2. epoll Reactor
3. ET + nonblocking socket
4. thread pool
5. connection state management
6. metrics subsystem
7. atomic optimization
8. remove hot path logging
9. fast path bypass worker

项目重点并非功能堆叠，

而是：

* 理解系统瓶颈
* 分析 latency 来源
* 观察 synchronization overhead
* 理解架构 tradeoff
* 学习真实系统优化过程

---

# 关键收获

## IO 并不是唯一瓶颈

系统性能问题不仅可能来自 socket IO，

还可能来自：

* logging
* synchronization
* scheduler latency
* context switch
* cache behavior

---

## thread pool 不一定降低 latency

thread pool 能够避免慢任务阻塞 Reactor，

但同时也会引入：

* scheduling overhead
* synchronization cost
* additional context switch

对于轻量 workload，

这些开销可能比业务处理本身更高。

---

## hot path 设计非常重要

请求热路径中的：

* `printf`
* unnecessary synchronization
* blocking syscall

都会显著影响 latency。

---

## ET 模式必须配合 non-blocking

ET 模式下，

socket 必须持续读取直到 `EAGAIN`。

否则可能导致：

* socket buffer 残留数据
* 后续事件无法触发
* 连接假死

---

# 未来优化方向

后续可以继续探索：

* per-thread queue
* object pool
* async logging
* zero-copy response
* io_uring support
* keep-alive connection support
* HTTP parser optimization

当前项目重点仍然是：

* Reactor 架构理解
* Linux 高并发 IO
* latency / throughput tradeoff
* 系统性能分析与优化
