## lock ##

**Interface:** `/hustmq/lock`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **on** (Required, 0 or 1)

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/lock?queue=test_queue&on=1"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**Return Example A2:**

	HTTP/1.1 200 OK

[Previous](../hustmq.md)

[Home](../../index.md)