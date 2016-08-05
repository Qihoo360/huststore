## put ##

**接口:** `/hustcache/ttl`

**方法:** `GET`

**参数:** 

*  **key** （必选）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustcache/ttl?key=test_key"

**结果范例A1:**

	HTTP/1.1 404 Not Found

**结果范例A2:**

	HTTP/1.1 200 OK
	Content-Length: 2
	Content-Type: text/plain

	45
	
[上一级](../hustdb.md)

[根目录](../../../index.md)