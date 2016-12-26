## zadd ##

**接口:** `/zadd`

**方法:** `GET | POST`

**参数:** 

*  **tb** （必选）  
*  **score** （必选）  
*  **key** （必选）  
*  **opt** （可选）
*  **ver** （可选）

该接口是 `/hustdb/zadd` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/zadd.md) 。

**使用范例:**

    curl -i -X POST "http://localhost:8082/zadd?tb=test_table&score=60" -d "test_key"

[上一级](../ha.md)

[回首页](../../index.md)