## pub ##

**接口:** `/hustmq/pub`

**方法:** `GET | POST`

**参数:**  

*  **queue** （必选）  
*  **item** （必选，GET：val即参数 or POST：val即body）  
*  **idx** （可选）
*  **ttl** （可选，default：900）    

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/pub?queue=test_queue&item=test_item"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue number exceeded the threshold

**结果范例A2:**

	HTTP/1.1 200 OK

[上一级](../hustmq.md)

[回首页](../../index.md)