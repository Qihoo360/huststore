## worker ##

**Interface:** `/hustmq/worker`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/worker?queue=test_queue"

**Return Example A1:**

	HTTP/1.1 200 OK
	Content-Length: 73
	Content-Type: text/plain

	[{"w":"s1.bjdt|9998","t":1458815637},{"w":"s1.bjdt|9999","t":1458815632}]

[Previous](../hustmq.md)

[Home](../../index.md)