## max ##

**Interface:** `/hustmq/max`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **num** (Required)

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/max?queue=test_queue&num=10000"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**Return Example A2:**

	HTTP/1.1 200 OK

[Previous](../hustmq.md)

[Home](../../index.md)