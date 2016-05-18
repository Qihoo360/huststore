`timeout`
----------

**接口:** `/hustmq/timeout`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **timeout** （必选，单位：秒）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/timeout?queue=test_queue&timeout=600"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**结果范例A2:**

	HTTP/1.1 200 OK

[上一级](../hustmq.md)

[根目录](../../index.md)