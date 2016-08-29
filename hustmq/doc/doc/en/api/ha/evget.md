## evget ##

**Interface:** `/evget`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **worker** (Required)  
*  **ack** (Optional)

该接口和 [put](put.md) 相配合实现 `long polling` 机制。

**使用范例:**

    curl -i -X GET "http://localhost:8080/evget?queue=test_queue&worker=test_worker"

[Previous](../ha.md)

[Home](../../index.md)