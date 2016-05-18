`ack`
----------

**接口:** `/hustmq/ack`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **token** （必选）

**使用范例:**

    curl -i -X GET "http://localhost:8085/hustmq/ack?queue=test_queue&token=0:1"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**结果范例A2:**

	HTTP/1.1 404 Not Found //already acked

**结果范例A3:**

	HTTP/1.1 200 OK

[上一级](../hustmq.md)

[根目录](../../index.md)