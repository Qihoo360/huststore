## zismember ##

**接口:** `/zismember`

**方法:** `GET | POST`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  

该接口是 `/hustdb/zismember` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/zismember.md) 。

**使用范例:**

    curl -i -X POST "http://localhost:8082/zismember?tb=test_table" -d "test_key"

[上一级](../ha.md)

[根目录](../../index.md)