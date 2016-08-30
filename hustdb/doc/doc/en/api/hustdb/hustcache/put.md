## put ##

**Interface:** `/hustcache/put`

**Method:** `GET | POST`

**Parameter:** 

*  **key** (Required)  
*  **val** (Required, GET：val is argument or POST：val is body)  
*  **ttl** (Optional, default：N/A)

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustcache/put?key=test_key&val=test_val"

**Result A1:**

	HTTP/1.1 200 OK
	
[Previous](../hustdb.md)

[Home](../../../index.md)