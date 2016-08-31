## srem ##

**Interface:** `/srem`

**Method:** `POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
This parameter must be put in `http body`.  
*  **ver** (Optional)

This interface is a proxy interface for `/hustdb/srem`. See more details in [here](../hustdb/hustdb/srem.md).  

**Sample:**

    curl -i -X POST "http://localhost:8082/srem?tb=test_table" -d "test_key"

[Previous](../ha.md)

[Home](../../index.md)