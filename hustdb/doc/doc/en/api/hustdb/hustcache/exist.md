## exist ##

**Interface:** `/hustcache/exist`

**Method:** `GET`

**Parameter:** 

*  **key** (Required)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustcache/exist?key=test_key"

**Result A1:**

	HTTP/1.1 404 Not Found
	
**Result A2:**

	HTTP/1.1 200 OK
	
[Previous](../hustdb.md)

[Home](../../../index.md)