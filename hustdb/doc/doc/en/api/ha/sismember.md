## sismember ##

**Interface:** `/sismember`

**Method:** `POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
This parameter must be put in `http body`.

This Interface is an proxy interface for `/hustdb/sismember`. See more details in [here](../hustdb/hustdb/sismember.md).  

**Sample:**

    curl -i -X POST "http://localhost:8082/sismember?tb=test_table" -d "test_key"

[Previous page](../ha.md)

[Home](../../index.md)