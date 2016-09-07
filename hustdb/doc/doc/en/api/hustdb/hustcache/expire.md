## expire ##

**Interface:** `/hustcache/expire`

**Method:** `GET`

**Parameter:** 

*  **key** (Required)    
*  **ttl** (Required)

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustcache/expire?key=test_key&ttl=60"

**Result A1:**

	HTTP/1.1 404 Not Found
		
**Result A2:**

	HTTP/1.1 200 OK
	
[Previous](../hustcache.md)

[Home](../../../index.md)