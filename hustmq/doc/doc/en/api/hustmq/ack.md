## ack ##

**Interface:** `/hustmq/ack`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)   
*  **token** (Required) 

**Example:**

    curl -i -X GET "http://localhost:8085/hustmq/ack?queue=test_queue&token=0:1"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist or no unacked message

**Return Example A2:**

	HTTP/1.1 404 Not Found //already acked

**Return Example A3:**

	HTTP/1.1 200 OK

[Previous](../hustmq.md)

[Home](../../index.md)