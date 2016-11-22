## do_get_status ##

**Interface:** `/do_get_status`

**Method:** `GET`

**Parameter:** 

N/A
  
This interface is used to fetch the status of task [`do_get`](do_get.md) in real time.

**Example:**

    curl -i -X GET 'localhost:8080/do_get_status'

**Return value:**

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Thu, 31 Mar 2016 03:14:49 GMT
    Content-Type: text/plain
    Content-Length: 72
    Connection: keep-alive
    
    {"unassigned_tasks":2,"assigned_tasks":0,"free_tasks":1022,"total":1024}

Note: `total = unassigned_tasks + assigned_tasks + free_tasks`

* unassigned_tasks: tasks delivered but not claimed by [`do_post`](do_post.md)
* assigned_tasks: tasks delivered and claimed [`do_post`](do_post.md) 
* free_tasks: the remaining amount of tasks that can be assigned, when a [`do_get`](do_get.md) request comes, its value will reduce by one, and the corresponding `unassigned_tasks` or `assigned_tasks` will increase by one.
* total: total size of task cache assigned.

[Previous](../ha.md)

[Home](../../index.md)