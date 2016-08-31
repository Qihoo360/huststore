## stat ##

**Interface:** `/hustmq/stat`

**Method:** `GET`

**Parameter:**  

*  **queue** (Required)   

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/stat?queue=test_queue"

**Return Example A1:**

	HTTP/1.1 200 OK
	Content-Length: 94
	Content-Type: text/plain

	{"queue":"test_queue","ready":[0,0,0],"unacked":1,"max":0,"lock":0,"type":0,"timeout":5,"si":1,"ci":1,"tm":1458812893}

[Previous](../hustmq.md)

[Home](../../index.md)