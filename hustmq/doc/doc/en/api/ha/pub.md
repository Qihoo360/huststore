## pub ##

**Interface:** `/pub`

**Method:** `GET | POST`

**Parameter:** 

*  **queue** (Required)  
*  **item** (Required)  
*  **idx** (Optional)
*  **ttl** (Optional)
  
该接口是 `/hustmq/pub` 的代理接口，参数详情可参考 [这里](../hustmq/pub.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/pub?queue=test_queue&item=test_value"

[Previous](../ha.md)

[Home](../../index.md)