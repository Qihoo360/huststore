## pub ##

**Interface:** `/pub`

**Method:** `GET | POST`

**Parameter:** 

*  **queue** (Required)  
*  **item** (Required)  
*  **idx** (Optional)
*  **ttl** (Optional)
  
This is proxy interface for `/hustmq/pub`, please refer to [Here](../hustmq/pub.md).

**Example:**

    curl -i -X GET "http://localhost:8080/pub?queue=test_queue&item=test_value"

[Previous](../ha.md)

[Home](../../index.md)