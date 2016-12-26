## persist ##

**接口:** `/cache/persist`

**方法:** `GET`

**参数:** 

*  **key** （必选）  

该接口是 `/hustcache/persist` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/persist.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/persist?key=test_key"
	
[上一页](../cache.md)

[回首页](../../../index.md)