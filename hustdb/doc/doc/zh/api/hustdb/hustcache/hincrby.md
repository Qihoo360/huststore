## hincrby ##

**接口:** `/hustcache/hincrby`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
*  **val** （必选）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustcache/hincrby?tb=test_table&key=test_key&val=7"

**结果范例A1:**

	HTTP/1.1 200 OK
	Content-Length: 2
	Content-Type: text/plain

	16 //if original value of test_key is 9, 9 + 7 = 16

[上一级](../hustcache.md)

[根目录](../../../index.md)