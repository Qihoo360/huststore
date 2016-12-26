## sub ##

**接口:** `/hustmq/sub`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **idx** （必选）  

**使用范例:**

    curl -i -X GET "http://localhost:8085/hustmq/sub?queue=test_queue&idx=0"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**结果范例A2:**

	HTTP/1.1 401 Unauthorized
	Index: 0-2 //idx must be >0 && <= 2

**结果范例A3:**

	HTTP/1.1 200 OK
	Content-Length: 9
	Content-Type: text/plain

	test_item

[上一页](../hustmq.md)

[回首页](../../index.md)