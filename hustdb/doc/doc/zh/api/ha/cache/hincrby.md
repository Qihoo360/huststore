## hincrby ##

**接口:** `/cache/hincrby`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
*  **val** （必选）  

该接口是 `/hustcache/hincrby` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/hincrby.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/hincrby?tb=test_table&key=test_key&val=7"

[上一级](../cache.md)

[回首页](../../../index.md)