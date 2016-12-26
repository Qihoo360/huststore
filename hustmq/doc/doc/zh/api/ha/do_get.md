## do_get ##

**接口:** `/do_get`

**方法:** `POST`

**参数:** 

*  **queue** （必选）  任务的名称
*  **args** （必选） 任务的参数，必须放在 `http body` 中
  
该接口用于远程投递任务。

**使用范例:**

    curl -i -X POST 'localhost:8080/do_get?queue=fibonacci' -d '10'

**返回样例:**

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Tue, 29 Mar 2016 08:53:48 GMT
    Content-Type: text/plain
    Content-Length: 2
    Connection: keep-alive
    
    50

更加详尽的测试代码可参考 `hustmq/ha/nginx/test/do_get.py`

[上一页](../ha.md)

[回首页](../../index.md)