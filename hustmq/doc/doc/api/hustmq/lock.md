`lock`
----------

**接口:** `/hustmq/lock`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **on** （必选）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/lock?queue=test_queue&on=1"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**结果范例A1:**

	HTTP/1.1 200 OK

[上一级](../hustmq.md)

[根目录](../../index.md)