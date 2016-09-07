## get ##

**Interface:** `/hustmq/get`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **worker** (Required)
*  **ack** (Optional, default: true)

**Example:**

    curl -i -X GET "http://localhost:8085/hustmq/get?queue=test_queue&worker=xx.xxx.9999"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**Return Example A2:**

	HTTP/1.1 404 Not Found //queue empty or locked

**Return Example A3:**

	HTTP/1.1 200 OK
	Content-Length: 9
	Content-Type: text/plain

	test_item

**Example:**

    curl -i -X GET "http://localhost:8085/hustmq/get?queue=test_queue&worker=xx.xxx.9999&ack=false"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**Return Example A2:**

	HTTP/1.1 404 Not Found //queue empty or locked

**Return Example A3:**

	HTTP/1.1 200 OK
	Content-Length: 9
	Content-Type: text/plain
	Ack-Token: 0:1 //parameter of the interface /hustmq/ack

	test_item

[Previous](../hustmq.md)

[Home](../../index.md)