## pub ##

**Interface:** `/hustmq/pub`

**Method:** `GET | POST`

**Parameter:**  

*  **queue** (Required)   
*  **item** (Required. GET: val is parameter; POST: val is body)   
*  **idx** (Optional) 
*  **ttl** (Optional, default: 900)    

**Example A:**

    curl -i -X GET "http://localhost:8085/hustmq/pub?queue=test_queue&item=test_item"

**Return Example A1:**

	HTTP/1.1 412 Precondition Failed //queue number exceeded the threshold

**Return Example A2:**

	HTTP/1.1 200 OK

[Previous](../hustmq.md)

[Home](../../index.md)