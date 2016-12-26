## hincrby ##

**接口:** `/hincrby`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
*  **val** （必选） 
*  **ttl** （可选）  
*  **ver** （可选）  

该接口是 `/hustdb/hincrby` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/hincrby.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/hincrby?tb=test_table&key=test_key&val=1"

[上一页](../ha.md)

[回首页](../../index.md)