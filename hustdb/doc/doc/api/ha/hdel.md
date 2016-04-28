`hdel`
----------

**接口:** `/hdel`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
*  **ver** （可选）  

该接口是 `/hustdb/hdel` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/hdel.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/hdel?tb=test_table&key=test_key"

[上一级](../ha.md)

[根目录](../../index.md)