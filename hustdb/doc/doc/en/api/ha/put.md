## put ##

**Interface:** `/put`

**Method:** `GET | PUT | POST`

**Parameter:** 

*  **key** (Required)  
*  **val** (Required)
*  **ttl** (Optional)
*  **ver** (Optional)  

This Interface is an proxy interface for `/hustdb/put`. See more details in [here](../hustdb/hustdb/put.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/put?key=test_key&val=test_val"

[Previous](../ha.md)

[Home](../../index.md)