## purge ##

**Interface:** `/hustmq/purge`

**Method:** `GET`

**Parameter:**  

*  **queue** (Required)  
*  **priori** (Required)    

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/purge?queue=test_queue&priori=0"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue number exceeded the threshold

**Return Example A2:**

	HTTP/1.1 200 OK

[Previous](../hustmq.md)

[Home](../../index.md)