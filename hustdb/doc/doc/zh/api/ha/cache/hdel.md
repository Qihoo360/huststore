## hdel ##

**接口:** `/cache/hdel`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  

该接口是 `/hustcache/hdel` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/hdel.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/hdel?tb=test_table&key=test_key"

[上一页](../cache.md)

[回首页](../../../index.md)