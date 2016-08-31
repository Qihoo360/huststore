## sub ##

**Interface:** `/sub`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **idx** (Required)  the index number of push data
  
This is the proxy interface of `/hustmq/sub`, please refer to [Here](../hustmq/sub.md).

**Example:**

    curl -i -X GET "http://localhost:8080/sub?queue=test_queue&idx=1"

[Previous](../ha.md)

[Home](../../index.md)