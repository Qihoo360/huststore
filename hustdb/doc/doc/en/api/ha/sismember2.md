## sismember2 ##

**Interface:** `/sismember2`

**Method:** `POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
This parameter must be put in `http body`.

This interface is a proxy interface for `/hustdb/sismember`. See more details in [here](../hustdb/hustdb/sismember.md).  

**Sample:**

    curl -i -X POST "http://localhost:8082/sismember2?tb=test_table" -d "test_key"

According to the consistency status of version, this interface will return the following values: 

**Return value 1(version is matched):**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:31:13 GMT
    Content-Type: text/plain
    Content-Length: 2
    Connection: keep-alive
    Version: 1

Related fields: 
  
* `Version`: version of the value 

**Return value 2(version is not matched):**

    HTTP/1.1 409 Conflict
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:33:09 GMT
    Content-Type: text/plain
    Content-Length: 5
    Connection: keep-alive
    Version1: 1
    Version2: 1

Related fields: 

* `Version1`: version of `master1`'s value.
* `Version2`: version of `master2`'s value. 

[Previous](../ha.md)

[Home](../../index.md)