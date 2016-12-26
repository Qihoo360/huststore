## exist ##

**接口:** `/hustdb/exist`

**方法:** `GET`

**参数:** 

*  **key** （必选）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/exist?key=test_key"

**结果范例A1:**

	HTTP/1.1 404 Not Found
	Version: 0
	
**结果范例A2:**

	HTTP/1.1 200 OK
	Version: 1
	
[上一页](../hustdb.md)

[回首页](../../../index.md)