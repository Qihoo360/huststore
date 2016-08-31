## hget ##

**Interface:** `/hget`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  

This interface is a proxy interface for `/hustdb/hget`. See more details in [here](../hustdb/hustdb/hget.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/hget?tb=test_table&key=test_key"

[Previous](../ha.md)

[Home](../../index.md)