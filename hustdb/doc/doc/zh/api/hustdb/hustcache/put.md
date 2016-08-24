## put ##

**接口:** `/hustcache/put`

**方法:** `GET | POST`

**参数:** 

*  **key** （必选）  
*  **val** （必选，GET：val即参数 or POST：val即body）  
*  **ttl** （可选，default：无）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustcache/put?key=test_key&val=test_val"

**结果范例A1:**

	HTTP/1.1 200 OK
	
[上一级](../hustdb.md)

[根目录](../../../index.md)