## expire ##

**接口:** `/cache/expire`

**方法:** `GET`

**参数:** 

*  **key** （必选）    
*  **ttl** （必选）

该接口是 `/hustcache/expire` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/expire.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/expire?key=test_key&ttl=60"
	
[上一级](../cache.md)

[回首页](../../../index.md)