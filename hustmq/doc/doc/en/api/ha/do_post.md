## do_post ##

**Interface:** `/do_post`

**Method:** `POST`

**Parameter:** 

**Scene 1**  (Claim a task)

N/A

**Example:**

    curl -i -X POST 'localhost:8080/do_post'

**Return value:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Tue, 29 Mar 2016 08:44:31 GMT
    Content-Type: text/plain
    Content-Length: 2
    Connection: keep-alive
    Queue: fibonacci
    Token: 1459234528278661
    
    10
In the returned `http` header, `Queue` represents the name of the task, `Token` is the identity of the task, it must be returned in along with the result. `10` is the argument of the task, should be put in `http body`.

**Scene 2**  (Return the process result)
  
*  **token** (Required)  the only identity of the task
*  **result** (Required)  process result, must be put in `http body`

**Example:**

    curl -i -X GET "http://localhost:8080/do_post?token=1459234528278661" -d "50"

**Return value:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Tue, 29 Mar 2016 08:53:48 GMT
    Content-Type: text/plain
    Content-Length: 0
    Connection: keep-alive
    
See more details in `hustmq/ha/nginx/test/do_post.py`

[Previous](../ha.md)

[Home](../../index.md)