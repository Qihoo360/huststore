## hdel ##

**Interface:** `/hdel`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **ver** (Optional)  

This Interface is an proxy interface for `/hustdb/hdel`. See more details in [here](../hustdb/hustdb/hdel.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/hdel?tb=test_table&key=test_key"

[Previous page](../ha.md)

[Home](../../index.md)