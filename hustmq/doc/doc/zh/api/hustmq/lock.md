## lock ##

**接口:** `/hustmq/lock`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **on** （必选，0 or 1）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/lock?queue=test_queue&on=1"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**结果范例A2:**

	HTTP/1.1 200 OK

[上一页](../hustmq.md)

[回首页](../../index.md)