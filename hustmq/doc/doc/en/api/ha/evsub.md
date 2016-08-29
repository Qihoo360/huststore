## evsub ##

**Interface:** `/evsub`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **idx** (Required)  the index of pushed data
  
This interface will work in cooperate with [sub](sub.md) and [pub](pub.md) to realize `http push` mechanism. The server will check `idx` when its subscribed at the first time, if it's not a legal value, the server will return code `403`, and will insert the correct index into `http` header ( The field's name is `Index`). 

Please see more details in test script:  
Script path: `hustmq/ha/nginx/test/autotest.py`

**Example:**

    curl -i -X GET "http://localhost:8080/evsub?queue=test_queue&idx=1"

[Previous](../ha.md)

[Home](../../index.md)