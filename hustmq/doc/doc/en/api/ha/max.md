## max ##

**Interface:** `/max`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **num** (Required)  

This is proxy interface for `/hustmq/max`, please refer to [Here](../hustmq/max.md).

**Example:**

    curl -i -X GET "http://localhost:8080/max?queue=test_queue&num=1000"

[Previous](../ha.md)

[Home](../../index.md)