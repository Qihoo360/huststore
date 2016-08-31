## worker ##

**Interface:** `/worker`

**Method:** `GET`

**Parameter:**  

*  **queue** (Required) 

This is proxy interface of `/hustmq/worker`, please refer to [here](../hustmq/worker.md).

**Example:**

    curl -i -X GET "http://localhost:8080/worker?queue=test_queue"

[Previous](../ha.md)

[Home](../../index.md)