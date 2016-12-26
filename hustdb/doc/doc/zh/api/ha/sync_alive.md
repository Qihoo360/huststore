## sync_alive ##

**接口:** `/sync_alive`

**方法:** `GET`

**参数:** 无

该接口用于判断 `sync server` 是否存活。

**使用范例:**

    curl -i -X GET "http://localhost:8082/sync_alive"

**返回值范例:**

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Tue, 07 Jun 2016 03:25:18 GMT
    Content-Type: text/plain
    Content-Length: 3
    Connection: keep-alive
    
    ok

[上一页](../ha.md)

[回首页](../../index.md)