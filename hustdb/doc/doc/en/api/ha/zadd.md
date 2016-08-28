## zadd ##

**Interface:** `/zadd`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **score** (Required)  
*  **key** (Required)  
*  **opt** (Optional)
*  **ver** (Optional)

This Interface is an proxy interface for `/hustdb/zadd`. See more details in [here](../hustdb/hustdb/zadd.md).  

**Sample:**

    curl -i -X POST "http://localhost:8082/zadd?tb=test_table&score=60" -d "test_key"

[Previous page](../ha.md)

[Root directory](../../index.md)