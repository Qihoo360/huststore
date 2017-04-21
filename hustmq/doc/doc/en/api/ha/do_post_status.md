## do_post_status ##

**Interface:** `/do_post_status`

**Method:** `GET`

**Parameter:** 

N/A
  
This interface is used to fetch [`do_post`](do_post.md) task status in real time. 

**Example:**

    curl -i -X GET 'localhost:8080/do_post_status'

**Return value:**

    HTTP/1.1 200 OK
    Server: nginx/1.12.0
    Date: Tue, 18 Apr 2017 03:40:06 GMT
    Content-Type: text/plain
    Content-Length: 52
    Connection: keep-alive
    
    {"cache_workers":2,"free_workers":1022,"total":1024}

* cache_workers: haven't yet claimed the `worker` of the delivered [`do_get`](do_get.md) task
* free_workers: the remaining number of `worker`s that can be assigned, each time a [`do_post`](do_post.md) request comes, this value will reduce by 1, and the corresponding `cache_executers` will increase by one.
* total: total size of `worker` cache assigned. 

[Previous](../ha.md)

[Home](../../index.md)