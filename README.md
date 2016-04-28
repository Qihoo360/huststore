# huststore - 高性能分布式存储服务 #
![huststore logo](res/logo.png)

`huststore` 是一个高性能的分布式存储服务，不但提供了 **`10w QPS`** 级别的 `kv` 存储的功能，还提供了 `hash`、`set` 等一系列数据结构的支持，并且支持 **二进制** 的 kv 存储，可以完全取代 `Redis` 的功能。此外，`huststore` 还结合特有的 `HA` 模块实现了分布式消息队列的功能，包括消息的流式推送，以及消息的 `发布-订阅` 等功能，可以完全取代 `RabbitMQ` 的功能。

## 特性 ##
`huststore` 分为 `hustdb` 以及 `HA` 模块两大部分。`hustdb` （存储引擎）的底层设计采用了自主开发的 `fastdb`，通过一套独特的 `md5 db` 将`QPS` 提升至 `10w` 级别的水准（含网络层的开销）。`HA` 以 `nginx` 模块的方式开发。`nginx` 是工业级的 `http server` 标准，得益于此，`huststore` 具备以下特性：
  
* 高吞吐量  
`hustdb` 的网络层采用了开源的 [libevhtp](https://github.com/ellzey/libevhtp) 来实现，结合自主研发的高性能 `fastdb` 存储引擎，性能测试 `QPS` 在 **10w** 以上。  
* 高并发  
参考 `nginx` 的并发能力。  
* 高可用性  
`huststore` 整体架构支持 `Replication` (master-master)，支持 `load balance` 。   
`HA` 的可用性由`nginx` 的 `master-worker` 架构所保证。当某一个 `worker` 意外挂掉时， `master` 会自动再启动一个 `worker` 进程，而且多个 `worker` 之间是相互独立的，从而保证了 `HA` 的高可用性。  
`huststore` 的高可用性由其整体架构特点保证。由于 `hustdb` 的存储节点采用了 `master-master` 的结构，当某一个存储节点挂掉时，`HA` 会自动将请求打到另外一台 `master`，同时 `HA` 会按照自动进行负载均衡，将数据分布存储在多个 `hustdb`节点上，因此存储引擎不存在单点限制。  
同时 `HA` 集群本身也是分布式的设计，而且每个 `HA` 节点都是独立的，当某一台 `HA` 挂掉时， LVS 会自动将请求打到其他可用的 `HA` 节点，从而解决了 `HA` 得单点限制。
* 通用性的接口   
`huststore` 使用 `http` 作为通用协议，因此客户端的实现不限制于语言。
* 支持二进制的 `key-value`

## 架构设计 ##

### 运维架构 ###
![architect](res/architect.png)

### 存储引擎设计 ###
![hustdb](res/hustdb.png)

## 依赖 ##
* [leveldb](https://github.com/google/leveldb)
* [libcurl](https://curl.haxx.se/libcurl/)
* [libevhtp](https://github.com/ellzey/libevhtp)
* [zlog](https://github.com/HardySimpson/zlog)

## 文档 ##

### 目录 ###
* [hustdb](hustdb/doc/doc/index.md)
* [hustmq](hustmq/doc/doc/index.md)

以上包含了 `huststore` 从设计、部署、`API` 到测试样例的详细文档，并提供了 `FAQ` 对常见问题进行快速检索。

### 快速入门 ###
* [hustdb](hustdb/doc/doc/guide/index.md)
* [hustmq](hustmq/doc/doc/guide/index.md)

### API ###
* [hustdb](hustdb/doc/doc/api/index.md)
* [hustmq](hustmq/doc/doc/api/index.md)

### 进阶 ###
* [hustdb](hustdb/doc/doc/advanced/index.md)
* [hustmq](hustmq/doc/doc/advanced/index.md)

### FAQ ###
* [hustdb](hustdb/doc/doc/appendix/faq.md)
* [hustmq](hustmq/doc/doc/appendix/faq.md)

## 目录结构 ##

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

**压测参数:** `100 concurrent，100w query`

**DB CONF:** `single instance，thread model，10 worker`

**测试结果:**

    （1）PUT
    	<1>value：256B；     qps：9.5w
	    <2>value：1KB；      qps：8.5w
	    <3>value：4KB；      qps：2.5w
	    <4>value：16KB；     qps：7k
	    <5>value：64KB；     qps：2k

	（2）GET
	    <1>value：256B；     qps：10w
	    <2>value：1KB；      qps：10w
	    <3>value：4KB；      qps：2.5w
	    <4>value：16KB；     qps：7k
	    <5>value：64KB；     qps：2k

	（3）DEL
    	<1>value：256B；     qps：10w
	    <2>value：1KB；      qps：10w
    	<3>value：4KB；      qps：10w
    	<4>value：16KB；     qps：10w
    	<5>value：64KB；     qps：10w

### `hustmq` ###

**机器配置:** `24core，64gb，1tb sata(7200rpm)`

**压测参数:** `100 concurrent，100w query，single queue`

**DB CONF:** `single instance，thread model，10 worker`

**测试结果:**

    （1）PUT
	    <1>item：256B；     qps：3w
	    <2>item：1KB；      qps：2.5w
	    <3>item：4KB；      qps：2w
	    <4>item：16KB；     qps：7k
	    <5>item：64KB；     qps：2k

	（2）GET
	    <1>item：256B；     qps：2.5w
	    <2>item：1KB；      qps：2w
	    <3>item：4KB；      qps：1.8w
	    <4>item：16KB；     qps：7k
	    <5>item：64KB；     qps：2k

	（3）STAT_ALL           qps：9.5w

## LICENSE ##

`huststore` is licensed under [New BSD License](https://opensource.org/licenses/BSD-3-Clause), a very flexible license to use.

## Authors ##

* hustxrb (hustxrb@163.com)  
* jobs (yao050421103@163.com)  