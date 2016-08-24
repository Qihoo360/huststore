## hset ##

**接口:** `/hustcache/hset`

**方法:** `GET | POST`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
*  **val** （必选，GET：val即参数 or POST：val即body）  

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustcache/hset?tb=test_table&key=test_key&val=test_val"

**结果范例A1:**

	HTTP/1.1 200 OK

[上一级](../hustdb.md)

[根目录](../../../index.md)