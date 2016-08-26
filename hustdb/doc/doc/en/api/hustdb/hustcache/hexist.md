## hexist ##

**Interface:** `/hustcache/hexist`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustcache/hexist?tb=test_table&key=test_key"

**Result A1:**

	HTTP/1.1 404 Not Found
	
**Result A2:**

	HTTP/1.1 200 OK

[Previous page](../hustdb.md)

[Root directory](../../../index.md)