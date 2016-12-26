## ttl ##

**接口:** `/cache/ttl`

**方法:** `GET`

**参数:** 

*  **key** （必选）  

该接口是 `/hustcache/ttl` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/ttl.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/ttl?key=test_key"
	
[上一级](../cache.md)

[回首页](../../../index.md)