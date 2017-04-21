## timeout ##

**Interface:** `/timeout`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **minute** (Required)  
  
This is proxy interface of `/hustmq/timeout`, please refer to [Here](../hustmq/timeout.md).

**Example:**

    curl -i -X GET "http://localhost:8080/timeout?queue=test_queue&minute=1"

**Return Example:**

    HTTP/1.1 200 OK
    Server: nginx/1.12.0
    Date: Mon, 17 Apr 2017 10:37:27 GMT
    Content-Type: text/plain
    Content-Length: 0
    Connection: keep-alive

[Previous](../ha.md)

[Home](../../index.md)