## timeout ##

**Interface:** `/hustmq/timeout`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **minute** (Required, Unit: minute, Range: 1 ~ 255)

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/timeout?queue=test_queue&minute=5"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**Return Example A2:**

	HTTP/1.1 200 OK

[Previous](../hustmq.md)

[Home](../../index.md)