## lock ##

**Interface:** `/lock`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **on** (Required)  

This is proxy interface for `/hustmq/lock`, please refer to [Here](../hustmq/lock.md).

**Example:**

    curl -i -X GET "http://localhost:8080/lock?queue=test_queue&on=1"

**Animation:**
![lock](../../../res/lock.gif)

[Previous](../ha.md)

[Home](../../index.md)