## hexist ##

**接口:** `/hexist`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  

该接口是 `/hustdb/hexist` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/hexist.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/hexist?tb=test_table&key=test_key"

[上一级](../ha.md)

[根目录](../../index.md)