[English](README.md)

# huststore - 高性能分布式存储服务 #
![huststore logo](res/logo.png)

`huststore` 是一个高性能的分布式存储服务，不但提供了 **`100 thousand QPS`** 级别的 `kv` 存储的功能，还提供了 `hash`、`set`、`sort set` 等一系列数据结构的支持，并且支持 **二进制** 的 kv 存储，可以替代 `Redis` 相关的功能。此外，`huststore` 还结合特有的 `HA` 模块实现了分布式消息队列的功能，包括消息的流式推送，以及消息的 `发布-订阅` 等功能，可以替代 `rabbitmq or gearman` 相关的功能。

## 特性 ##
`huststore` 分为 `hustdb` 以及 `HA` 模块两大部分。`hustdb` （存储引擎）的底层设计采用了自主开发的 `fastdb`。`HA` 以 `nginx` 模块的方式开发。`nginx` 是工业级的 `http server` 标准，得益于此，`huststore` 具备以下特性：
  
* 高吞吐量  
`hustdb` 的网络层采用了开源的 [libevhtp](https://github.com/ellzey/libevhtp) 来实现，结合自主研发的高性能 `fastdb` 存储引擎，性能测试 `QPS` 在 **100 thousand** 以上。
* 高并发  
参考 `nginx` 的并发能力。  
* 高可用性  
`huststore` 整体架构支持 `Replication` (master-master)，支持 `load balance` 。   
`HA` 的可用性由`nginx` 的 `master-worker` 架构所保证。当某一个 `worker` 意外挂掉时， `master` 会自动再启动一个 `worker` 进程，而且多个 `worker` 之间是相互独立的，从而保证了 `HA` 的高可用性。  
`huststore` 的高可用性由其整体架构特点保证。由于 `hustdb` 的存储节点采用了 `master-master` 的结构，当某一个存储节点挂掉时，`HA` 会自动将请求打到另外一台 `master`；同时 `HA` 会自动进行负载均衡，将数据分散存储在多个 `hustdb`节点上，因此存储引擎不存在单点限制。  
同时 `HA` 集群本身也是分布式的设计，而且每个 `HA` 节点都是独立的，当某一台 `HA` 挂掉时， LVS 会自动将请求打到其他可用的 `HA` 节点，从而解决了 `HA` 得单点限制。
* 通用性的接口   
`huststore` 使用 `http` 作为通用协议，因此客户端的实现不限制于语言。
* 支持二进制的 `key-value`

## 运维 ##

### 架构 ###
![architect](res/architect.png)

### 部署 ###
* 分布式KV存储 = HA（hustdb ha） + DB（hustdb）
* 分布式消息队列 = HA（hustmq ha） + DB（hustdb）

## 存储引擎(fastdb) ##
![hustdb](res/hustdb.png)

## 依赖 ##
* [cmake](https://cmake.org/download/)
* [leveldb](https://github.com/google/leveldb)
* [libcurl](https://curl.haxx.se/libcurl/)
* [libevent](http://libevent.org/)
* [libevhtp](https://github.com/ellzey/libevhtp)
* [zlog](https://github.com/HardySimpson/zlog)

## 文档 ##

### 目录 ###
* [hustdb](hustdb/doc/doc/zh/index.md)
* [hustmq](hustmq/doc/doc/zh/index.md)

以上包含了 `huststore` 从设计、部署、`API` 到测试样例的详细文档，并提供了 `FAQ` 对常见问题进行快速检索。

### 快速入门 ###
* [hustdb](hustdb/doc/doc/zh/guide/index.md)
* [hustmq](hustmq/doc/doc/zh/guide/index.md)

### API ###
* [hustdb](hustdb/doc/doc/zh/api/index.md)
* [hustmq](hustmq/doc/doc/zh/api/index.md)

### 进阶 ###
* [hustdb](hustdb/doc/doc/zh/advanced/index.md)
* [hustmq](hustmq/doc/doc/zh/advanced/index.md)

### FAQ ###
* [hustdb](hustdb/doc/doc/zh/appendix/faq.md)
* [hustmq](hustmq/doc/doc/zh/appendix/faq.md)

## 目录 ##

项目目录结构如下：

`hustdb`  
　　`doc`  
　　`db`  
　　`ha`  
　　`sync`    
`hustmq`  
　　`doc`  
　　`ha`  

`hustdb/ha` 服务于存储引擎，可以配置多个 `worker`。  
`hustmq/ha` 服务于消息队列，**只能配置单个 `worker`**。

## 性能 ##

### `hustdb` ###

**机器配置:** `24core，64gb，1tb sata(7200rpm)`

**压测参数:** `100 concurrent，1000 thousand querys`

**DB CONF:** `single instance，thread model，10 workers`

**测试结果:**

    （1）PUT
    	<1>value：256B；     qps：95 thousand
	    <2>value：1KB；      qps：85 thousand
	    <3>value：4KB；      qps：25 thousand
	    <4>value：16KB；     qps：7 thousand
	    <5>value：64KB；     qps：2 thousand

	（2）GET
	    <1>value：256B；     qps：100 thousand
	    <2>value：1KB；      qps：10 thousand
	    <3>value：4KB；      qps：25 thousand
	    <4>value：16KB；     qps：7 thousand
	    <5>value：64KB；     qps：2 thousand

	（3）DEL
    	<1>value：256B；     qps：100 thousand
	    <2>value：1KB；      qps：100 thousand
    	<3>value：4KB；      qps：100 thousand
    	<4>value：16KB；     qps：100 thousand
    	<5>value：64KB；     qps：100 thousand

### `hustmq` ###

**机器配置:** `24core，64gb，1tb sata(7200rpm)`

**压测参数:** `100 concurrent，1000 thousand querys，single queue`

**DB CONF:** `single instance，thread model，10 workers`

**测试结果:**

    （1）PUT
	    <1>item：256B；     qps：30 thousand
	    <2>item：1KB；      qps：25 thousand
	    <3>item：4KB；      qps：20 thousand
	    <4>item：16KB；     qps：7 thousand
	    <5>item：64KB；     qps：2 thousand

	（2）GET
	    <1>item：256B；     qps：25 thousand
	    <2>item：1KB；      qps：20 thousand
	    <3>item：4KB；      qps：18 thousand
	    <4>item：16KB；     qps：7 thousand
	    <5>item：64KB；     qps：2 thousand

## LICENSE ##

`huststore` is licensed under [New BSD License](https://opensource.org/licenses/BSD-3-Clause), a very flexible license to use.

## Authors ##

* XuRuibo（hustxrb，hustxrb@163.com)  
* ChengZhuo（jobs，yao050421103@163.com)  