## purge ##

**Interface:** `/purge`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **priori** (Required)  

该接口是 `/hustmq/purge` 的代理接口，参数详情可参考 [这里](../hustmq/max.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/purge?queue=test_queue&priori=0"

[Previous](../ha.md)

[Home](../../index.md)