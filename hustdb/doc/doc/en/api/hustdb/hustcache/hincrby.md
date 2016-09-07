## hincrby ##

**Interface:** `/hustcache/hincrby`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **val** (Required)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustcache/hincrby?tb=test_table&key=test_key&val=7"

**Result A2:**

	HTTP/1.1 200 OK
	Content-Length: 2
	Content-Type: text/plain

	16 //if original value of test_key is 9, 9 + 7 = 16

[Previous](../hustcache.md)

[Home](../../../index.md)