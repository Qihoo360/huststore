## max ##

**接口:** `/hustmq/max`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **num** （必选）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/max?queue=test_queue&num=10000"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**结果范例A2:**

	HTTP/1.1 200 OK

[上一级](../hustmq.md)

[根目录](../../index.md)