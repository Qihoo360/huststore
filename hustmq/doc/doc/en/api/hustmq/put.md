## put ##

**Interface:** `/hustmq/put`

**Method:** `GET | POST`

**Parameter:**  

*  **queue** (Required)  
*  **item** (Required. GET: val is parameter; POST: val is body) 
*  **priori** (Optional, 0~2, default: 0)  

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/put?queue=test_queue&item=test_item"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue number exceeded the threshold

**Return Example A2:**

	HTTP/1.1 200 OK

[Previous](../hustmq.md)

[Home](../../index.md)