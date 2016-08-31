## hdel ##

**Interface:** `/cache/hdel`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  

This interface is a proxy interface for `/hustcache/hdel`. See more details in [here](../../hustdb/hustcache/hdel.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/cache/hdel?tb=test_table&key=test_key"

[Previous](../cache.md)

[Home](../../../index.md)