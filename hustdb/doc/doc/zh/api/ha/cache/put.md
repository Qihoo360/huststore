## put ##

**接口:** `/cache/put`

**方法:** `GET | PUT | POST`

**参数:** 

*  **key** （必选）  
*  **val** （必选）  
*  **ttl** （可选）

该接口是 `/hustcache/put` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/put.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/put?key=test_key&val=test_val"
	
[上一页](../cache.md)

[回首页](../../../index.md)