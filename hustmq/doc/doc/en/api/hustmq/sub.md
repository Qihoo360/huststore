## sub ##

**Interface:** `/hustmq/sub`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)   
*  **idx** (Required)   

**Example:**

    curl -i -X GET "http://localhost:8085/hustmq/sub?queue=test_queue&idx=0"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue not exist

**Return Example A2:**

	HTTP/1.1 401 Unauthorized
	Index: 0-2 //idx must be >0 && <= 2

**Return Example A3:**

	HTTP/1.1 200 OK
	Content-Length: 9
	Content-Type: text/plain

	test_item

[Previous](../hustmq.md)

[Home](../../index.md)