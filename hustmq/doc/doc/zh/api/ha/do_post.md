## do_post ##

**接口:** `/do_post`

**方法:** `POST`

**参数:** 

**场景 1**  （认领任务）

无

**使用范例:**

    curl -i -X POST 'localhost:8080/do_post'

**返回样例:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Tue, 29 Mar 2016 08:44:31 GMT
    Content-Type: text/plain
    Content-Length: 2
    Connection: keep-alive
    Queue: fibonacci
    Token: 1459234528278661
    
    10
返回的 `http` 头部中，`Queue` 代表任务的名字，`Token` 是本次任务处理过程的唯一标识符，在返回处理结果的时候必须带上。`10` 是任务的参数，在 `http body` 中。

**场景 2**  （返回处理结果）
  
*  **token** （必选）  任务的唯一标识符号
*  **result** （必选）  处理结果，必须放在 `http body` 中

**使用范例:**

    curl -i -X GET "http://localhost:8080/do_post?token=1459234528278661" -d "50"

**返回样例:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Tue, 29 Mar 2016 08:53:48 GMT
    Content-Type: text/plain
    Content-Length: 0
    Connection: keep-alive
    
更加详尽的测试代码可参考 `hustmq/ha/nginx/test/do_post.py`

[上一级](../ha.md)

[根目录](../../index.md)