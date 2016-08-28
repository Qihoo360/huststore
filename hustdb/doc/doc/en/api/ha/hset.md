## hset ##

**Interface:** `/hset`

**Method:** `GET | PUT | POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **val** (Required)  
*  **ttl** (Optional)  
*  **ver** (Optional)  

This Interface is an proxy interface for `/hustdb/hset`. See more details in [here](../hustdb/hustdb/hset.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/hset?tb=test_table&key=test_key&val=test_val"

[Previous page](../ha.md)

[Root directory](../../index.md)