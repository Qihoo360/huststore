hustmq
--

### 接口概要 ###

以下是 `hustmq` 提供的 http 接口：

* [stat_all](hustmq/stat_all.md)
* [put](hustmq/put.md)
* [get](hustmq/get.md)
* [lock](hustmq/lock.md)
* [max](hustmq/max.md)
* [purge](hustmq/purge.md)
* [worker](hustmq/worker.md)
* [pub](hustmq/pub.md)
* [sub](hustmq/sub.md)

### 参数说明 ###

#### `queue` ####
队列名称   

#### `item` ####
消息数据

#### `priori` ####
消息优先级，0~2，优先级由低至高

#### `worker` ####
worker信息，用于get接口，保存worker相关数据

#### `ttl` ####
time to live，存活时间（过期删除），0表示永久存储

#### `on` ####
lock开关，0~1，限制队列put操作

#### `num` ####
max数值，限制队列最大消息数量

#### `idx` ####
订阅消息滑动窗口索引值

[上一级](index.md)

[根目录](../index.md)