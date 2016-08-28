## append ##

**Interface:** `/hustcache/append`

**Method:** `GET | PUT | POST`

**Parameter:** 

*  **key** (Required)  
*  **val** (Required)  

This Interface is an proxy interface for `/hustcache/append`. See more details in [here](../../hustdb/hustcache/append.md). 

**Sample:**

    curl -i -X GET "http://localhost:8082/cache/append?key=test_key&val=test_val"
	
[Previous page](../cache.md)

[Root directory](../../../index.md)