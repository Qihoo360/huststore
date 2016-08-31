## put ##

**Interface:** `/cache/put`

**Method:** `GET | PUT | POST`

**Parameter:** 

*  **key** (Required)  
*  **val** (Required)  
*  **ttl** (Optional)

This interface is a proxy interface for `/hustcache/put`. See more details in [here](../../hustdb/hustcache/put.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/cache/put?key=test_key&val=test_val"
	
[Previous](../cache.md)

[Home](../../../index.md)