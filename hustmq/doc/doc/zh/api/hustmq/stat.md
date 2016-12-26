## stat ##

**接口:** `/hustmq/stat`

**方法:** `GET`

**参数:**  

*  **queue** （必选）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustmq/stat?queue=test_queue"

**结果范例A1:**

	HTTP/1.1 200 OK
	Content-Length: 94
	Content-Type: text/plain

	{"queue":"test_queue","ready":[0,0,0],"unacked":1,"max":0,"lock":0,"type":0,"timeout":5,"si":1,"ci":1,"tm":1458812893}

[上一级](../hustmq.md)

[回首页](../../index.md)