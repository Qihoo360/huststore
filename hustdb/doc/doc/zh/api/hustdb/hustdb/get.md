## get ##

**接口:** `/hustdb/get`

**方法:** `GET`

**参数:** 

*  **key** （必选）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/get?key=test_key"

**结果范例A1:**

	HTTP/1.1 404 Not Found
	Version: 0

**结果范例A2:**

	HTTP/1.1 200 OK
	Version: 1
	Content-Length: 8
	Content-Type: text/plain

	test_val

[上一级](../hustdb.md)

[回首页](../../../index.md)