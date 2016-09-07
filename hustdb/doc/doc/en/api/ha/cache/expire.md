## expire ##

**Interface:** `/cache/expire`

**Method:** `GET`

**Parameter:** 

*  **key** (Required)    
*  **ttl** (Required)

This Interface is a proxy interface for `/hustcache/expire`. See more details in [here](../../hustdb/hustcache/expire.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/cache/expire?key=test_key&ttl=60"
	
[Previous](../cache.md)

[Home](../../../index.md)