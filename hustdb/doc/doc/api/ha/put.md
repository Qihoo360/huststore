## put ##

**接口:** `/put`

**方法:** `GET | PUT | POST`

**参数:** 

*  **key** （必选）  
*  **val** （必选）
*  **ttl** （可选）
*  **ver** （可选）  

该接口是 `/hustdb/put` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/put.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/put?key=test_key&val=test_val"

[上一级](../ha.md)

[根目录](../../index.md)