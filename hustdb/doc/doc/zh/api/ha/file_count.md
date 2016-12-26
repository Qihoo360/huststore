## file_count ##

**接口:** `/file_count`

**方法:** `GET`

**参数:** 

*  **peer** （必选）  
表示后端机节点索引，参考 [peer_count](peer_count.md)  

该接口是 `/hustdb/file_count` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/file_count.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/file_count?peer=0"

[上一级](../ha.md)

[回首页](../../index.md)