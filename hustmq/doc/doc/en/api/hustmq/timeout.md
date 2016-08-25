## timeout ##

**接口:** `/hustmq/timeout`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **minute** （必选，单位：分钟，范围：1 ~ 255）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/timeout?queue=test_queue&minute=5"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**结果范例A2:**

	HTTP/1.1 200 OK

[上一级](../hustmq.md)

[根目录](../../index.md)