`put`
----------

**接口:** `/hustdb/put`

**方法:** `GET | POST`

**参数:** 

*  **key** （必选）  
*  **val** （必选，GET：val即参数 or POST：val即body）  
*  **ttl** （可选，default：0）
*  **ver** （可选，default：0）    

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/put?key=test_key&val=test_val"

**结果范例A1:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/put?key=test_key&val=test_val&ver=2"

**结果范例B1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true
	
[上一级](../hustdb.md)

[根目录](../../../index.md)