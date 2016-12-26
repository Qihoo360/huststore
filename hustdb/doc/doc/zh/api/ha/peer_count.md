## peer_count ##

**接口:** `/peer_count`

**方法:** `GET`

**参数:** 无

该接口用于获取后端 `hustdb` 节点个数。依赖于该接口的 `API` 包括：  

* [stat](stat.md)
* [stat_all](stat_all.md)
* [keys](keys.md)
* [hkeys](hkeys.md)
* [smembers](smembers.md)
* [export](export.md)
* [file_count](file_count.md)

以上接口均需要传入 `peer` 参数。假设 `peer_count` 的值为 **N**，则该参数的值的范围为 **[0, N-1]** 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/peer_count"

[上一级](../ha.md)

[回首页](../../index.md)