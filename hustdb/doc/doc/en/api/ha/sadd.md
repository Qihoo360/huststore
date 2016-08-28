## sadd ##

**Interface:** `/sadd`

**Method:** `POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
This parameter must be put in `http body`.
*  **ver** (Optional)

This Interface is an proxy interface for `/hustdb/sadd`. See more details in [here](../hustdb/hustdb/sadd.md).  

**Sample:**

    curl -i -X POST "http://localhost:8082/sadd?tb=test_table" -d "test_key"

[Previous page](../ha.md)

[Root directory](../../index.md)