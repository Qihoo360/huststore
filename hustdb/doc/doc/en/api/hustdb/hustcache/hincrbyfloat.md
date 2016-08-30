## hincrbyfloat ##

**Interface:** `/hustcache/hincrbyfloat`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **val** (Required)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustcache/hincrbyfloat?tb=test_table&key=test_key&val=5.9"

**Result A2:**

	HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	7.5 //if original value of test_key is 1.7, 1.7 + 5.8 = 7.5

[Previous](../hustdb.md)

[Home](../../../index.md)