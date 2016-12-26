## pub ##

**接口:** `/pub`

**方法:** `GET | POST`

**参数:** 

*  **queue** （必选）  
*  **item** （必选）  
*  **idx** （可选）
*  **ttl** （可选）
  
该接口是 `/hustmq/pub` 的代理接口，参数详情可参考 [这里](../hustmq/pub.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/pub?queue=test_queue&item=test_value"

[上一页](../ha.md)

[回首页](../../index.md)