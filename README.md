[简体中文](README_ZH.md)

# huststore - High-performance Distributed Storage #
![huststore logo](res/logo.png)

`huststore` is a open source high performance distributed database system. It not only provides key-value storage service with extremely high performance, up to 100 thousand QPS, but also supports data structures like `hash`, `set`, `sorted set`, etc. Also, it can store **binary** data as value from a key-value pair, and thus can be used as an alternative of Redis.

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
`huststroe` use `http` as the communication protocol, therefore the client side implementation is not limited in any specific programming language.

* Support Binary Key-Value  

## Operation and Maintenance ##

### Architect ###
![architect](res/architect.png)

### Deployment ###
* Distributed KV storage  = HA（hustdb ha） + DB（hustdb）
* Distributed Message Queue  = HA（hustmq ha） + DB（hustdb）

## Database Engine ##
![hustdb](res/hustdb.png)

## Dependency ##
* [cmake](https://cmake.org/download/)
* [leveldb](https://github.com/google/leveldb)
* [libcurl](https://curl.haxx.se/libcurl/)
* [libevent2](http://libevent.org/)
* [libevhtp](https://github.com/ellzey/libevhtp)
* [zlog](https://github.com/HardySimpson/zlog)

## Documents ##

### Catalog ###
* [hustdb](hustdb/doc/doc/en/index.md)
* [hustmq](hustmq/doc/doc/en/index.md)

Above includes detailed documents of design, deployments, `API` usage and test samples. You can refer quickly to common problems in `FAQ` part.

### Guide ###
* [hustdb](hustdb/doc/doc/en/guide/index.md)
* [hustmq](hustmq/doc/doc/en/guide/index.md)

### API manual ###
* [hustdb](hustdb/doc/doc/en/api/index.md)
* [hustmq](hustmq/doc/doc/en/api/index.md)

### Advanced ###
* [hustdb](hustdb/doc/doc/en/advanced/index.md)
* [hustmq](hustmq/doc/doc/en/advanced/index.md)

### FAQ ###
* [hustdb](hustdb/doc/doc/en/appendix/faq.md)
* [hustmq](hustmq/doc/doc/en/appendix/faq.md)

## Table Content ##

`hustdb`  
　　`doc`  
　　`db`  
　　`ha`  
　　`sync`    
`hustmq`  
　　`doc`  
　　`ha`  

`hustdb/ha` provides service for storage engine, could configured with multiple `worker`s.  
`hustmq/ha` provides service for message queue, **can only configured with one `worker`**.

## Performance ##

### Environment ###

    CPU: Intel(R) Xeon(R) CPU E5-2630 @ 2.30GHz (6cores x2)
    Memory: 64G
    Disk: Intel SSD DC S3500 Series (300GB, 2.5in SATA 6Gb/s, 20nm, MLC), x4, RAID10(softraid), SAS Controller: LSI Logic SAS2008 PCI-Express Fusion-MPT SAS-2
    Network Adapter: Intel I350
    OS: CentOS release 6.8 x86_64 (2.6.32-642.4.2.el6.x86_64)

### Products ###

* [redis 3.2.6](https://redis.io/)
* [ssdb 1.9.4](http://ssdb.io)
* [hustdb 1.5](https://github.com/Qihoo360/huststore)

### Tools ###

* [redis-benchmark](https://redis.io/topics/benchmarks)
* [wrk](https://github.com/wg/wrk)

### Arguments ###

abbr         |concurrency |value (bytes)
-------------|------------|--------------
C1000-V256   |1000        |256
C1000-V512   |1000        |512
C1000-V1024  |1000        |1024
C2000-V256   |2000        |256
C2000-V512   |2000        |512
C2000-V1024  |2000        |1024

### Benchmark ###

#### PUT ####

![benchmark_put](res/benchmark_put.png)

#### GET ####

![benchmark_get](res/benchmark_get.png)

See more details in [here](benchmark/README.md)

## LICENSE ##

`huststore` is licensed under [New BSD License](https://opensource.org/licenses/BSD-3-Clause), a very flexible license to use.

## Authors ##

* XuRuibo（hustxrb, hustxrb@163.com)  
* ChengZhuo（jobs, yao050421103@163.com)  