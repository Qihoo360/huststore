## max ##

**Interface:** `/max`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **num** (Required)  

该接口是 `/hustmq/max` 的代理接口，参数详情可参考 [这里](../hustmq/max.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/max?queue=test_queue&num=1000"

[Previous](../ha.md)

[Home](../../index.md)