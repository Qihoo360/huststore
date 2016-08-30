## zscore ##

**Interface:** `/zscore`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  

This Interface is an proxy interface for `/hustdb/zscore`. See more details in [here](../hustdb/hustdb/zscore.md).  

**Sample:**

    curl -i -X POST "http://localhost:8082/zscore?tb=test_table" -d "test_key"

[Previous](../ha.md)

[Home](../../index.md)