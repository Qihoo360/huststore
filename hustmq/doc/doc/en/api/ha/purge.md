## purge ##

**Interface:** `/purge`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **priori** (Required)  

This is proxy interface for `/hustmq/purge`, please refer to [Here](../hustmq/max.md).

**Example:**

    curl -i -X GET "http://localhost:8080/purge?queue=test_queue&priori=0"

[Previous](../ha.md)

[Home](../../index.md)