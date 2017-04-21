## sync_alive ##

**Interface:** `/sync_alive`

**Method:** `GET`

**Parameter:** 

This interface is used to detect whether `sync server` is alive.

**Sample:**

    curl -i -X GET "http://localhost:8082/sync_alive"

**Return value:**

    HTTP/1.1 200 OK
    Server: nginx/1.12.0
    Date: Tue, 18 Apr 2017 03:25:18 GMT
    Content-Type: text/plain
    Content-Length: 3
    Connection: keep-alive
    
    ok

[Previous](../ha.md)

[Home](../../index.md)