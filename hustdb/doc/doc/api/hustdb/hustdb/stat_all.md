## stat_all ##

**接口:** `/hustdb/stat_all`

**方法:** `GET`

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/stat_all"

**结果范例A1:**

	HTTP/1.1 200 OK
	Content-Length: 74
	Content-Type: text/plain

	[{"table":"","type":-1,"size":3},{"table":"test_table","type":2,"size":1}]

[上一级](../hustdb.md)

[根目录](../../../index.md)