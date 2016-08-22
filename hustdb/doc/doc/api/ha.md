ha
--

### 接口概要 ###

以下是 `hustdb ha` 提供的 http 接口：

* [get_table](ha/get_table.md)
* [set_table](ha/set_table.md)
* [peer_count](ha/peer_count.md)
* [sync_status](ha/sync_status.md)
* [sync_alive](ha/sync_alive.md)
* [put](ha/put.md)
* [get](ha/get.md)
* [get2](ha/get2.md)
* [del](ha/del.md)
* [exist](ha/exist.md)
* [keys](ha/keys.md)
* [hset](ha/hset.md)
* [hget](ha/hget.md)
* [hget2](ha/hget2.md)
* [hdel](ha/hdel.md)
* [hexist](ha/hexist.md)
* [hkeys](ha/hkeys.md)
* [sadd](ha/sadd.md)
* [srem](ha/srem.md)
* [sismember](ha/sismember.md)
* [smembers](ha/smembers.md)
* [zadd](ha/zadd.md)
* [zscore](ha/zscore.md)
* [zscore2](ha/zscore2.md)
* [zrem](ha/zrem.md)
* [zismember](ha/zismember.md)
* [zrangebyrank](ha/zrangebyrank.md)
* [zrangebyscore](ha/zrangebyscore.md)
* [stat](ha/stat.md)
* [stat_all](ha/stat_all.md)
* [file_count](ha/file_count.md)
* [`cache`](ha/cache.md)
    * [exist](ha/cache/exist.md)
    * [get](ha/cache/get.md)
    * [ttl](ha/cache/ttl.md)
    * [put](ha/cache/put.md)
    * [append](ha/cache/append.md)
    * [del](ha/cache/del.md)
    * [expire](ha/cache/expire.md)
    * [persist](ha/cache/persist.md)
    * [hexist](ha/cache/hexist.md)
    * [hget](ha/cache/hget.md)
    * [hset](ha/cache/hset.md)
    * [hdel](ha/cache/hdel.md)
    * [hincrby](ha/cache/hincrby.md)
    * [hincrbyfloat](ha/cache/hincrbyfloat.md)

### 常见问题 ###

* 测试代码  
所有接口的测试代码的写法均可以在测试脚本中找到。  
脚本路径：`hustdb/ha/nginx/test/`

* 返回值  
所有接口通过 `http code` 来表示执行结果，`200` 表示执行成功，`404` 表示执行失败。

* 关于 `http basic authentication`  
本节所有以 `curl` 命令描述的测试样例均假设 `http basic authentication` 被关闭。  
具体做法可参考 [这一节](../advanced/ha/nginx.md) 的末尾对常见问题的解答。

* `hustdb` 采用了最终一致性的设计，因此所有的写操作的处理方式为：
	* `master1` 和 `master2` 均写入成功，返回 `http code` 为 `200`
	* `master1` 和 `master2` 只有一台写入成功，返回 `http code` 为 `200`，但会在 `http` 头部加入 `Sync` 字段，值为写入失败，需要进行数据同步的机器（可参考测试脚本的做法： `hustdb/ha/nginx/test/autotest.py`）
	* `master1` 和 `master2` 均写入失败，返回 `http code` 为 `404`  

* 关于 `get2`，`hget2`，`zscore2`  
    * `get2` 和 `get` 的差别在于： **数据可靠性，性能** 。（另外两组接口类似）  
    * `get2` 会将 `master1` 和 `master2` 节点的数据都取回来，进行比较，只有版本和值都一致，才返回 `200`，同时在 `http` 头部加上 `Version` 字段。如果版本不一致，则会将两个版本均添加到`http` 头部。如果值不一致，则会返回 `409`，同时将两个不一致的值打包在 `http body` 中返回，具体可参考 [get2](ha/get2.md) 。
    * `get` 的做法是，先读取 `master1` 的数据，如果获取成功就直接返回给客户端；如果读取 `master1` 的数据失败，则读取`master2` 的数据，如果读取成功则返回 `master2` 的数据。
    * 综合比较，`get2` 获取的数据是强一致的，但由于需要串行访问 `master1` 和 `master2`，因此 `QPS` 会比 `get` 低很多；`get` 获取的数据是弱一致的，但由于大部分情况下它只需要访问 `master1` 即可拿到数据，因此 `QPS` 会比 `get2` 高很多。
    * 结论：`get` 是 **弱一致性，高 `QPS`** ；`get2` 是 **强一致性，低 `QPS`** 。如果业务需要保证数据的强一致性，同时对吞吐量没有很高的要求，可以选用 `get2`，例如金融类业务；如果业务需要很高的吞吐量，可以忍受部分读取数据的不一致，则 `get` 更合适，例如商品信息展示。

[上一级](index.md)

[根目录](../index.md)