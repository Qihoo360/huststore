## zscore2 ##

**Interface:** `/zscore2`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  

This Interface is an proxy interface for `/hustdb/zscore`. See more details in [here](../hustdb/hustdb/zscore.md).  

**Sample:**

    curl -i -X POST "http://localhost:8082/zscore2?tb=test_table" -d "test_key"

According to the version of data and consistency status of the value, this interface will return the following values: 

**Return value 1(Both value and version are matched):**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:31:13 GMT
    Content-Type: text/plain
    Content-Length: 2
    Connection: keep-alive
    Version: 1
    
    60

Related fields: 
  
* `Version`: version of the value 

**Return value 2 (Value is matched, version is not matched):**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:31:41 GMT
    Content-Type: text/plain
    Content-Length: 2
    Connection: keep-alive
    Version1: 1
    Version2: 2
    
    60

Related fields: 

* `Version1`: version of `master1`'s value.
* `Version2`: version of `master2`'s value. 

**Return value 3 (Value is not matched, version is matched)**

    HTTP/1.1 409 Conflict
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:32:11 GMT
    Content-Type: text/plain
    Content-Length: 5
    Connection: keep-alive
    Version: 1
    Val-Offset: 2
    
    60100

Related fields: 

* `Version`: version  
* `Val-Offset`: cut point of the two values. If we cut `60100` at the offset `2`, we get two sub values, `60` and `100`, representing the values stored at `master1` and `master2` separately. 

**Return value 4 (Both value and version are not matched):**

    HTTP/1.1 409 Conflict
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:33:09 GMT
    Content-Type: text/plain
    Content-Length: 5
    Connection: keep-alive
    Version1: 1
    Version2: 1
    Val-Offset: 2
    
    60100

Related fields: 

* `Version1`: version of `master1`'s value.  
* `Version2`: version of `master2`'s value.
* `Val-Offset`: cut point of the two values. See more details in `Return value 3`.

[Previous page](../ha.md)

[Root directory](../../index.md)