## hincrbyfloat ##

**接口:** `/hustcache/hincrbyfloat`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
*  **val** （必选）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustcache/hincrbyfloat?tb=test_table&key=test_key&val=5.9"

**结果范例A2:**

	HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	7.5 //if original value of test_key is 1.7, 1.7 + 5.8 = 7.5

[上一级](../hustcache.md)

[根目录](../../../index.md)