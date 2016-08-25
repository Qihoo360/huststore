## do_get_status ##

**接口:** `/do_get_status`

**方法:** `GET`

**参数:** 

无
  
该接口用于实时获取 [`do_get`](do_get.md) 任务的状态。

**使用范例:**

    curl -i -X GET 'localhost:8080/do_get_status'

**返回样例:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 31 Mar 2016 03:14:49 GMT
    Content-Type: text/plain
    Content-Length: 72
    Connection: keep-alive
    
    {"unassigned_tasks":2,"assigned_tasks":0,"free_tasks":1022,"total":1024}

备注：`total = unassigned_tasks + assigned_tasks + free_tasks`

* unassigned_tasks: 表示已经投递，但尚未被 [`do_post`](do_post.md) 认领的任务
* assigned_tasks: 表示已经投递，且被 [`do_post`](do_post.md) 认领的任务
* free_tasks: 剩余可被分配的任务数量，每来一个 [`do_get`](do_get.md) 请求，该值都会减一，相应的 `unassigned_tasks` 或者 `assigned_tasks` 会加一
* total: 分配的任务缓存总数

[上一级](../ha.md)

[根目录](../../index.md)