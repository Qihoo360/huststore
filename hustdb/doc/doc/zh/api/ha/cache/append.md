## append ##

**接口:** `/hustcache/append`

**方法:** `GET | PUT | POST`

**参数:** 

*  **key** （必选）  
*  **val** （必选）  

该接口是 `/hustcache/append` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/append.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/append?key=test_key&val=test_val"
	
[上一页](../cache.md)

[回首页](../../../index.md)