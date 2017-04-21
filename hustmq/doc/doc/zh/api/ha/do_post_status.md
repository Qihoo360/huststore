## do_post_status ##

**接口:** `/do_post_status`

**方法:** `GET`

**参数:** 

无
  
该接口用于实时获取 [`do_post`](do_post.md) 任务的状态。

**使用范例:**

    curl -i -X GET 'localhost:8080/do_post_status'

**返回样例:**

    HTTP/1.1 200 OK
    Server: nginx/1.12.0
    Date: Tue, 18 Apr 2017 03:40:06 GMT
    Content-Type: text/plain
    Content-Length: 52
    Connection: keep-alive
    
    {"cache_workers":2,"free_workers":1022,"total":1024}

* cache_workers: 表示尚未认领 [`do_get`](do_get.md) 投递的任务的 `worker`
* free_workers: 剩余可被分配的 `worker` 数量，每来一个 [`do_post`](do_post.md) 请求，该值都会减一，相应的 `cache_executers` 会加一
* total: 分配的 `worker` 缓存总数

[上一页](../ha.md)

[回首页](../../index.md)