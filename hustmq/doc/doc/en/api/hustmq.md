hustmq
--

### API Synopsis ###

This following is http interface provided by `hustmq`:

* [stat_all](hustmq/stat_all.md)
* [put](hustmq/put.md)
* [get](hustmq/get.md)
* [ack](hustmq/ack.md)
* [timeout](hustmq/timeout.md)
* [lock](hustmq/lock.md)
* [max](hustmq/max.md)
* [purge](hustmq/purge.md)
* [worker](hustmq/worker.md)
* [pub](hustmq/pub.md)
* [sub](hustmq/sub.md)

### Arguments ###

#### `queue` ####
Queue name  

#### `item` ####
Message 

#### `ack` ####
Whether confirm it immediately(deleted) when fetching data

#### `token` ####
Used for asynchronous confirmation of message(deleted) 

#### `priori` ####
Message priority, 0~2 From low to high priority

#### `worker` ####
Worker name used for get interface

#### `ttl` ####
Time to live (delete it if expired), and 0 means permanent storage

#### `minute` ####
Set up queue message timeout, Unit: minute


#### `on` ####
Lock switch. 0 means turn off lock, and put operation is allowed; 1 means turn on lock, and put operation is not allowed.

#### `num` ####
The maximum message number for this queue

#### `idx` ####
Sliding window index of subscribe message queue 


[Previous](index.md)

[Home](../index.md)