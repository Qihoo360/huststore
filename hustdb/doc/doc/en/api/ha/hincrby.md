## hincrby ##

**Interface:** `/hincrby`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **val** (Required)  
*  **ttl** (Optional)  
*  **ver** (Optional)  

This interface is a proxy interface for `/hustdb/hincrby`. See more details in [here](../hustdb/hustdb/hincrby.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/hincrby?tb=test_table&key=test_key&val=1"

[Previous](../ha.md)

[Home](../../index.md)