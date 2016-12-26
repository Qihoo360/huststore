## put ##

**接口:** `/hustmq/put`

**方法:** `GET | POST`

**参数:**  

*  **queue** （必选）  
*  **item** （必选，GET：val即参数 or POST：val即body）  
*  **priori** （可选，0~2，default：0）    

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/put?queue=test_queue&item=test_item"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue number exceeded the threshold

**结果范例A2:**

	HTTP/1.1 200 OK

[上一页](../hustmq.md)

[回首页](../../index.md)