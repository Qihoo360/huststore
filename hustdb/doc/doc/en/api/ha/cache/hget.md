## hget ##

**Interface:** `/cache/hget`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  

This Interface is an proxy interface for `/hustcache/hget`. See more details in [here](../../hustdb/hustcache/hget.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/cache/hget?tb=test_table&key=test_key"

[Previous](../cache.md)

[Home](../../../index.md)