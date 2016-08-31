## evget ##

**Interface:** `/evget`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **worker** (Required)  
*  **ack** (Optional)

This interface will work in cooperate with [put](put.md) to realize http `long polling` mechanism. 

**Example:**

    curl -i -X GET "http://localhost:8080/evget?queue=test_queue&worker=test_worker"

[Previous](../ha.md)

[Home](../../index.md)