hustdb
--

### 接口概要 ###

以下是 `hustdb` 提供的 http 接口：

* [`hustdb`](hustdb/hustdb.md)
	* [stat](hustdb/hustdb/stat.md)
	* [stat_all](hustdb/hustdb/stat_all.md)
	* [file_count](hustdb/hustdb/file_count.md)
	* [export](hustdb/hustdb/export.md)
	* [exist](hustdb/hustdb/exist.md)
	* [get](hustdb/hustdb/get.md)
	* [put](hustdb/hustdb/put.md)
	* [del](hustdb/hustdb/del.md)
	* [keys](hustdb/hustdb/keys.md)
	* [hexist](hustdb/hustdb/hexist.md)
	* [hget](hustdb/hustdb/hget.md)
	* [hset](hustdb/hustdb/hset.md)
	* [hdel](hustdb/hustdb/hdel.md)
	* [hkeys](hustdb/hustdb/hkeys.md)
	* [sismember](hustdb/hustdb/sismember.md)
	* [sadd](hustdb/hustdb/sadd.md)
	* [srem](hustdb/hustdb/srem.md)
	* [smembers](hustdb/hustdb/smembers.md)
	* [zismember](hustdb/hustdb/zismember.md)
	* [zscore](hustdb/hustdb/zscore.md)
	* [zadd](hustdb/hustdb/zadd.md)
	* [zrem](hustdb/hustdb/zrem.md)
	* [zrangebyrank](hustdb/hustdb/zrangebyrank.md)
	* [zrangebyscore](hustdb/hustdb/zrangebyscore.md)

### 参数说明 ###

#### `tb` ####
表名称，用于hash，set   

#### `key` ####
key值

#### `val` ####
value值

#### `ttl` ####
time to live，存活时间（过期删除），0表示永久存储

#### `ver` ####
version，版本号，写操作传入ver必须与存储ver一致，0表示强制写

#### `file` ####
fastdb编号，0~N-1（N为hustdb.conf中fastdb.count值）

#### `start` ####
一致性hash起点，0~1024，默认0

#### `end` ####
一致性hash终点，0~1024，默认1024

#### `noval` ####
是否携带value值，用于export，keys等接口，默认true，即不携带

#### `cover` ####
是否覆盖现有snapshot，用于export接口，默认false，即不覆盖

#### `offset` ####
起点偏移量，用于keys，hkeys等接口，获取流式数据

#### `size` ####
记录条数，与offset配合使用，用于keys，hkeys等接口，获取流式数据

#### `score` ####
仅用于zadd，存储sort set键值分数

#### `opt` ####
仅用于zadd，0：score参数覆盖键值分数，1：score参数与键值分数相加后覆盖，-1：score参数与键值分数相减后覆盖

#### `min` ####
仅用于zrangebyscore，score下限

#### `max` ####
仅用于zrangebyscore，score上限

#### `async` ####
通过snapshot获取数据，snapshot由export接口产生，用于keys，hkeys等接口，默认false，即不使用snapshot

[上一级](index.md)

[根目录](../index.md)