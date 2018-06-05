[English](README.md)

# huststore - 高性能分布式存储服务 #
![huststore logo](res/logo.png)

`huststore` 是一个高性能的分布式存储服务，不但提供了 **`100 thousand QPS`** 级别的 `kv` 存储的功能，还提供了 `hash`、`set`、`sort set` 等一系列数据结构的支持，并且支持 **二进制** 的 kv 存储，可以替代 `Redis` 相关的功能。此外，`huststore` 还结合特有的 `HA` 模块实现了分布式消息队列的功能，包括消息的流式推送，以及消息的 `发布-订阅` 等功能，可以替代 `rabbitmq or gearman` 相关的功能。

## 特性 ##
`huststore` 分为 `hustdb` 以及 `HA` 模块两大部分。`hustdb`是自主研发的存储引擎。`HA` 以 `nginx` 模块的方式开发。`nginx` 是工业级的 `http server` 标准，得益于此，`huststore` 具备以下特性：
  
* 高吞吐量  
`hustdb` 的网络层采用了开源的 [libevhtp](https://github.com/ellzey/libevhtp) 来实现，结合自主研发的高性能存储引擎，性能测试 `QPS` 在 **数十万** 以。
* 高并发  
参考 `nginx` 的并发能力。  
* 高可用性  
`huststore` 整体架构支持 `Replication` (master-master)，支持 `load balance` 。   
`HA` 的可用性由`nginx` 的 `master-worker` 架构所保证。当某一个 `worker` 意外挂掉时， `master` 会自动再启动一个 `worker` 进程，而且多个 `worker` 之间是相互独立的，从而保证了 `HA` 的高可用性。  
`huststore` 的高可用性由其整体架构特点保证。由于 `hustdb` 的存储节点采用了 `master-master` 的结构，当某一个存储节点挂掉时，`HA` 会自动将请求打到另外一台 `master`；同时 `HA` 会自动进行负载均衡，将数据分散存储在多个 `hustdb`节点上，因此存储引擎不存在单点限制。  
同时 `HA` 集群本身也是分布式的设计，而且每个 `HA` 节点都是独立的，当某一台 `HA` 挂掉时， LVS 会自动将请求打到其他可用的 `HA` 节点，从而解决了 `HA` 得单点限制。
* 通用性的接口   
`huststore` 使用 `http` 作为通用协议，因此客户端的实现不限制于语言。  
* 持久化  
几乎所有的接口都会将数据持久化到硬盘，因此 **你不用再担心数据的丢失**。  
* 支持二进制的 `key-value`  
* 支持 `Version Clock`  

## 运维 ##

### 架构 ###
![architect](res/architect.png)

### 部署 ###
* 分布式KV存储 : HA (hustdb ha) + DB (hustdb)
* 分布式消息队列 : HA (hustmq ha) + DB (hustdb)

## 存储引擎 ##
![hustdb](res/hustdb.png)

## 平台 ##

目前测试通过的平台包括：

平台             | 描述
-----------------|----------------------------------------------------------
CentOS 6.x & 7.x | 内核版本 >= 2.6.32 (GCC 4.4.7)

## 依赖 ##
* [cmake](https://cmake.org/download/)
* [leveldb](https://github.com/google/leveldb)
* [libcurl](https://curl.haxx.se/libcurl/)
* [libevent2](http://libevent.org/)
* [libevhtp](https://github.com/ellzey/libevhtp)
* [zlog](https://github.com/HardySimpson/zlog)
* [zlib](https://zlib.net/)

## 快速入门 ##

请查看[这里](quickstart_zh.md)。

## 文档 ##

* [hustdb](hustdb/doc/doc/zh/index.md)
* [hustmq](hustmq/doc/doc/zh/index.md)

以上包含了 `huststore` 从设计、部署、`API` 到测试样例的详细文档，并提供了 `FAQ` 对常见问题进行快速检索。

## 性能 ##

### 测试环境 ###

    CPU: Intel(R) Xeon(R) CPU E5-2683 v4 @ 2.10GHz (2socket*16cores)
    内存: 192G
    磁盘: Intel SSD DC S3520 Series (800GB, 2.5in SATA 6Gb/s, 3D1, MLC)
    网卡: Intel Ethernet 10G 2P X520 Adapter
    系统: CentOS Linux release 7.2.1511 (3.10.0-327.el7.x86_64)

### 测试场景 1 -  Worker ###

#### 测试目的 ####

测试 `hustdb` 在不同 `worker` 线程数量下，其 `QPS` 上限。  

#### 测试条件 ####

storage capacity : 512GB  

concurrent connection : 1000

value : **1KB**

md5db cache : **禁用**  

data compression : **禁用**  

`CPU` 未绑定

#### 测试结果 ####

说明：横轴代表 `hustdb` 线程数，纵轴代表 `QPS`，value为1KB。  

![benchmark_workers](res/benchmark_workers.png)

#### 结论 ####

从以上测试图可以看出，`hustdb` 的 `worker` 线程数设置为36-40比较划算。

### 测试场景 2 -  RTT ###

#### 测试目的 ####

测试在最佳 `worker` 线程数（36）下，`hustdb` 的 `RTT` 表现。  

#### 测试条件 ####

storage capacity : 512GB  

concurrent connection : 200

value : **1KB**

md5db cache : **禁用**  

data compression : **禁用**  

`CPU` 未绑定

#### 测试结果 ####

    # GET
    24 threads and 200 connections
    Thread Stats   Avg      Stdev     Max   +/- Stdev
        Latency   238.94us   95.42us   9.33ms   81.03%
        Req/Sec    31.44k     1.09k   36.30k    63.68%
    Latency Distribution
        50%  220.00us
        75%  288.00us
        90%  343.00us
        99%  482.00us
    11333230 requests in 15.10s, 11.10GB read
    Requests/sec: 750635.81
    Transfer/sec:    752.65MB
    --------------------------------------------------
    [Latency Distribution]  0.01%  0.07ms
    [Latency Distribution]   0.1%  0.07ms
    [Latency Distribution]   0.5%  0.09ms
    [Latency Distribution]     1%  0.09ms
    [Latency Distribution]     3%  0.12ms
    [Latency Distribution]     5%  0.13ms
    [Latency Distribution]    10%  0.15ms
    [Latency Distribution]    20%  0.17ms
    [Latency Distribution]    30%  0.19ms
    [Latency Distribution]    40%  0.20ms
    [Latency Distribution]    50%  0.22ms
    [Latency Distribution]    60%  0.25ms
    [Latency Distribution]    70%  0.28ms
    [Latency Distribution]    80%  0.30ms
    [Latency Distribution]    90%  0.34ms
    [Latency Distribution]    91%  0.35ms
    [Latency Distribution]    92%  0.36ms
    [Latency Distribution]    93%  0.37ms
    [Latency Distribution]  93.5%  0.37ms
    [Latency Distribution]    94%  0.38ms
    [Latency Distribution]  94.5%  0.38ms
    [Latency Distribution]    95%  0.39ms
    [Latency Distribution]  95.5%  0.39ms
    [Latency Distribution]    96%  0.40ms
    [Latency Distribution]  96.5%  0.40ms
    [Latency Distribution]    97%  0.41ms
    [Latency Distribution]  97.5%  0.42ms
    [Latency Distribution]    98%  0.43ms
    [Latency Distribution]  98.5%  0.45ms
    [Latency Distribution]    99%  0.48ms
    [Latency Distribution]  99.1%  0.49ms
    [Latency Distribution]  99.2%  0.50ms
    [Latency Distribution]  99.3%  0.51ms
    [Latency Distribution]  99.4%  0.52ms
    [Latency Distribution]  99.5%  0.53ms
    [Latency Distribution]  99.6%  0.56ms
    [Latency Distribution]  99.7%  0.59ms
    [Latency Distribution]  99.8%  0.64ms
    [Latency Distribution]  99.9%  0.76ms
    [Latency Distribution]  99.99%  1.85ms
    [Latency Distribution]  99.999%  4.07ms

    # PUT
    24 threads and 200 connections
    Thread Stats   Avg      Stdev     Max   +/- Stdev
        Latency   495.13us  393.71us  21.29ms   93.06%
        Req/Sec    16.37k     1.33k   23.72k    74.26%
    Latency Distribution
        50%  447.00us
        75%  623.00us
        90%  815.00us
        99%    1.28ms
    17628712 requests in 45.10s, 1.53GB read
    Socket errors: connect 0, read 192, write 0, timeout 0
    Requests/sec: 390880.11
    Transfer/sec:     34.67MB
    --------------------------------------------------
    [Latency Distribution]  0.01%  0.09ms
    [Latency Distribution]   0.1%  0.10ms
    [Latency Distribution]   0.5%  0.12ms
    [Latency Distribution]     1%  0.12ms
    [Latency Distribution]     3%  0.14ms
    [Latency Distribution]     5%  0.17ms
    [Latency Distribution]    10%  0.20ms
    [Latency Distribution]    20%  0.26ms
    [Latency Distribution]    30%  0.33ms
    [Latency Distribution]    40%  0.39ms
    [Latency Distribution]    50%  0.45ms
    [Latency Distribution]    60%  0.51ms
    [Latency Distribution]    70%  0.58ms
    [Latency Distribution]    80%  0.67ms
    [Latency Distribution]    90%  0.81ms
    [Latency Distribution]    91%  0.84ms
    [Latency Distribution]    92%  0.86ms
    [Latency Distribution]    93%  0.89ms
    [Latency Distribution]  93.5%  0.90ms
    [Latency Distribution]    94%  0.92ms
    [Latency Distribution]  94.5%  0.93ms
    [Latency Distribution]    95%  0.95ms
    [Latency Distribution]  95.5%  0.97ms
    [Latency Distribution]    96%  0.99ms
    [Latency Distribution]  96.5%  1.02ms
    [Latency Distribution]    97%  1.05ms
    [Latency Distribution]  97.5%  1.08ms
    [Latency Distribution]    98%  1.13ms
    [Latency Distribution]  98.5%  1.19ms
    [Latency Distribution]    99%  1.28ms
    [Latency Distribution]  99.1%  1.30ms
    [Latency Distribution]  99.2%  1.33ms
    [Latency Distribution]  99.3%  1.37ms
    [Latency Distribution]  99.4%  1.41ms
    [Latency Distribution]  99.5%  1.47ms
    [Latency Distribution]  99.6%  1.56ms
    [Latency Distribution]  99.7%  1.73ms
    [Latency Distribution]  99.8%  2.24ms
    [Latency Distribution]  99.9%  4.23ms
    [Latency Distribution]  99.99%  7.22ms
    [Latency Distribution]  99.999%  9.62ms


### 测试场景 3 -  vs Redis ###

### 版本 ###

* [redis 4.0.9](https://redis.io/)

#### 测试工具 ####

* [redis-benchmark](https://redis.io/topics/benchmarks)
* [wrk](https://github.com/wg/wrk)

#### 测试条件 ####

storage capacity : 512GB  

md5db cache : **禁用**  

data compression : **禁用**  

`CPU` 未绑定

#### 参数说明 ####

缩写        |并发连接    |数据大小
-----------|------------|--------------
C1000-512B |1000        |512B
C1000-1K   |1000        |1KB
C1000-4K   |1000        |4KB
C2000-512B |2000        |512B
C2000-1K   |2000        |1KB
C2000-4K   |2000        |4KB

#### PUT ####

![benchmark_put](res/benchmark_put.png)

#### GET ####

![benchmark_get](res/benchmark_get.png)

详情请参考[这里](benchmark/README_ZH.md)

## LICENSE ##

`huststore` is licensed under [LGPL-3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html), a very flexible license to use.

## Authors ##

* XuRuibo（hustxrb，hustxrb@163.com)  
* ChengZhuo（jobs，yao050421103@163.com)  

## 更多 ##

- Nginx 模块开发工具包 - [hustngx](https://github.com/jobs-github/hustngx)