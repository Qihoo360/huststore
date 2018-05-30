[简体中文](README_ZH.md)

# huststore - High-performance Distributed Storage #
![huststore logo](res/logo.png)

`huststore` is a open source high performance distributed database system. It not only provides key-value storage service with extremely high performance, up to **100 thousand QPS**, but also supports data structures like `hash`, `set`, `sorted set`, etc. Also, it can store **binary** data as value from a key-value pair, and thus can be used as an alternative of Redis.

In addtion, `huststore` implements a distributed message queue by integrating a special `HA` module, features including message Push Stream, and message Publish-SubScribe, these features can be used as replacements of the corresponding features in rabbitmq and gearman.

## Features ##
`huststore` has two core components, `hustdb` and `HA`. `hustdb` is a database engine developed by our own, in the fundamental architecture. `HA` is implemented as a `nginx` module. It is well-known that `nginx` is a industry-proven high quality code base, thus by inheriting it `huststore` gains the below advantages:

* High Throughput  
`hustdb` uses [libevhtp](https://github.com/ellzey/libevhtp), a open source network library, as the inner network communication system, by incorporating it with high-performance storage engine, `hustdb` achieves a extremely high performance, the benchmark shows that `QPS` hits **100 thousand** and even more.

* High Concurrency  
Please refer to concurrency report of `nginx` for more details.

* High Availability  
`huststore` architecture provides `Replication` (master-master) and `load balance` support. Therefore, the availability of `HA` is guaranteed by `master-worker` design. When one of `worker` process is down, the `master` will load another `workder` process, since multiple `worker`s work independently, the `HA` is guaranteed to work steadily.
The fundamental design architecture of `huststore` guarantees the high availability, by using `master-master` architecture, when one of the storage node fails, `HA` module will re-direct the request to another living `master` node. Also, when a node failure happens, `HA` cluster will automatically re-balance the data distribution, thus avoid single point of failure.
In addition, `HA` cluster uses a distributed architecture design by incorporating LVS as the director, each `HA` node is separated and work independently. When one of the `HA` node is down, `LVS` will re-direct the request to other available `HA` node, thus avoids `HA`'s failure on single point node.

* Language-free Interface  
`huststore` use `http` as the communication protocol, therefore the client side implementation is not limited in any specific programming language.

* Persistence  
**You do not need to worry about the loss of data** as most of interfaces will persist data to disk.  

* Support Binary Key-Value  
* Support Version Clock

## Operation and Maintenance ##

### Architect ###
![architect](res/architect.png)

### Deployment ###
* Distributed KV storage  : HA (hustdb ha) + DB (hustdb)
* Distributed Message Queue  : HA (hustmq ha) + DB (hustdb)

## Database Engine ##
![hustdb](res/hustdb.png)

## Dependency ##
* [cmake](https://cmake.org/download/)
* [leveldb](https://github.com/google/leveldb)
* [libcurl](https://curl.haxx.se/libcurl/)
* [libevent2](http://libevent.org/)
* [libevhtp](https://github.com/ellzey/libevhtp)
* [zlog](https://github.com/HardySimpson/zlog)
* [zlib](https://zlib.net/)

## Platforms ##

Tested platforms so far:

Platform   | Description
-----------|----------------------------------------------------------
CentOS 6.x | kernel >= 2.6.32 (GCC 4.4.7)

## Quick Start ##

Read the [Quick Start](quickstart.md).

## Documents ##

* [hustdb](hustdb/doc/doc/en/index.md)
* [hustmq](hustmq/doc/doc/en/index.md)

Above includes detailed documents of design, deployments, `API` usage and test samples. You can refer quickly to common problems in `FAQ` part.

## Performance ##

### Environment ###

    CPU: Intel(R) Xeon(R) CPU E5-2683 v4 @ 2.10GHz (2socket*16cores)
    Memory: 192G
    Disk: Intel SSD DC S3520 Series (800GB, 2.5in SATA 6Gb/s, 3D1, MLC)
    Network Adapter: Intel Ethernet 10G 2P X520 Adapter
    OS: CentOS Linux release 7.2.1511 (3.10.0-327.el7.x86_64)

### Products ###

* [redis 4.0.9](https://redis.io/)
* [hustdb](https://github.com/Qihoo360/huststore)

### Tools ###

* [redis-benchmark](https://redis.io/topics/benchmarks)
* [wrk](https://github.com/wg/wrk)

### Arguments ###

abbr       |concurrency |value (KB)
-----------|------------|--------------
C1000-1K   |1000        |1
C1000-4K   |1000        |4
C1000-16K  |1000        |16
C2000-1K   |2000        |1
C2000-4K   |2000        |4
C2000-16K  |2000        |16

### Benchmark ###

#### PUT ####

![benchmark_put](res/benchmark_put.png)

#### GET ####

![benchmark_get](res/benchmark_get.png)

See more details in [here](benchmark/README.md)

## LICENSE ##

`huststore` is licensed under [LGPL-3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html), a very flexible license to use.

## Authors ##

* XuRuibo（hustxrb, hustxrb@163.com)  
* ChengZhuo（jobs, yao050421103@163.com)  

## More ##

- Nginx module development kit - [hustngx](https://github.com/jobs-github/hustngx)