## do_get ##

**Interface:** `/do_get`

**Method:** `POST`

**Parameter:** 

*  **queue** (Required)  the name of task
*  **args** (Required) the arguments of task, must be put in `http body`
  
This interface is used to deliver task remotely.

**Example:**

    curl -i -X POST 'localhost:8080/do_get?queue=fibonacci' -d '10'

**Return value:**

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Tue, 29 Mar 2016 08:53:48 GMT
    Content-Type: text/plain
    Content-Length: 2
    Connection: keep-alive
    
    50

See more details of test script in `hustmq/ha/nginx/test/do_get.py`

[Previous](../ha.md)

[Home](../../index.md)