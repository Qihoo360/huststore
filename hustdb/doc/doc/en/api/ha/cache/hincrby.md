## hincrby ##

**Interface:** `/cache/hincrby`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **val** (Required)  

This interface is a proxy interface for `/hustcache/hincrby`. See more details in [here](../../hustdb/hustcache/hincrby.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/cache/hincrby?tb=test_table&key=test_key&val=7"

[Previous](../cache.md)

[Home](../../../index.md)