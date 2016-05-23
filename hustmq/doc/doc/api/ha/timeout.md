`timeout`
----------

**接口:** `/timeout`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **minute** （必选）  
  
该接口是 `/hustmq/timeout` 的代理接口，参数详情可参考 [这里](../hustmq/timeout.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/timeout?queue=test_queue&minute=1"

**返回范例:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Mon, 23 May 2016 10:37:27 GMT
    Content-Type: text/plain
    Content-Length: 0
    Connection: keep-alive

[上一级](../ha.md)

[根目录](../../index.md)