## stat ##

**接口:** `/hustdb/stat`

**方法:** `GET`

**参数:** 

*  **tb** （可选；返回kv键值对数量，或者，返回对应table的键值对数量）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/stat"

**结果范例A1:**

	HTTP/1.1 200 OK
	Content-Length: 1
	Content-Type: text/plain

	1

[上一级](../hustdb.md)

[根目录](../../../index.md)