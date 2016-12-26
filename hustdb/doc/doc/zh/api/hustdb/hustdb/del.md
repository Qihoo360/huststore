## del ##

**接口:** `/hustdb/del`

**方法:** `GET`

**参数:** 

*  **key** （必选）  
*  **ver** （可选，default：0）  


**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/del?key=test_key"

**结果范例A1:**

	HTTP/1.1 404 Not Found
	Version: 0
	VerError: false
		
**结果范例A2:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/del?key=test_key&ver=2"

**结果范例B1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

**结果范例B2:**

	HTTP/1.1 200 OK
	Version: 2
	VerError: false

[上一级](../hustdb.md)

[回首页](../../../index.md)