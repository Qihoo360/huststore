## worker ##

**Interface:** `/worker`

**Method:** `GET`

**Parameter:**  

*  **queue** (Required) 

This interface is the proxy interface for `/hustmq/worker`. See more details in [here](../hustmq/worker.md)。

**Example**

    curl -i -X GET "http://localhost:8080/worker?queue=test_queue"

[Previous](../ha.md)

[Home](../../index.md)