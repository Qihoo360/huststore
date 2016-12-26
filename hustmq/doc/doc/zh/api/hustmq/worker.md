## worker ##

**接口:** `/hustmq/worker`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/wrker?queue=test_queue"

**结果范例A1:**

	HTTP/1.1 200 OK
	Content-Length: 73
	Content-Type: text/plain

	[{"w":"s1.bjdt|9998","t":1458815637},{"w":"s1.bjdt|9999","t":1458815632}]

[上一页](../hustmq.md)

[回首页](../../index.md)