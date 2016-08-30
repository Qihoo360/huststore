## stat_all ##

**Interface:** `/hustdb/stat_all`

**Method:** `GET`

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/stat_all"

**Result A1:**

	HTTP/1.1 200 OK
	Content-Length: 74
	Content-Type: text/plain

	[{"table":"","type":-1,"size":3},{"table":"test_table","type":2,"size":1}]

[Previous](../hustdb.md)

[Home](../../../index.md)