ha
--

### 接口概要 ###

以下是 `hustdb ha` 提供的 http 接口：

* [get_table](ha/get_table.md)
* [set_table](ha/set_table.md)
* [peer_count](ha/peer_count.md)
* [sync_status](ha/sync_status.md)
* [put](ha/put.md)
* [get](ha/get.md)
* [del](ha/del.md)
* [exist](ha/exist.md)
* [keys](ha/keys.md)
* [hset](ha/hset.md)
* [hget](ha/hget.md)
* [hdel](ha/hdel.md)
* [hexist](ha/hexist.md)
* [hkeys](ha/hkeys.md)
* [sadd](ha/sadd.md)
* [srem](ha/srem.md)
* [sismember](ha/sismember.md)
* [smembers](ha/smembers.md)
* [zadd](ha/zadd.md)
* [zscore](ha/zscore.md)
* [zrem](ha/zrem.md)
* [zismember](ha/zismember.md)
* [zrangebyrank](ha/zrangebyrank.md)
* [zrangebyscore](ha/zrangebyscore.md)
* [stat](ha/stat.md)
* [stat_all](ha/stat_all.md)
* [file_count](ha/file_count.md)

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

[上一级](index.md)

[根目录](../index.md)