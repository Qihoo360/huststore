## get2 ##

**Interface:** `/get2`

**Method:** `GET`

**Parameter:** 

*  **key** (Required)  

This interface is a proxy interface for `/hustdb/get`. See more details in [here](../hustdb/hustdb/get.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/get2?key=test_key"

According to the version of data and consistency status of the value, this interface will return the following values: 

**Return value 1 (Both value and version are matched):**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:05:31 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive
    Version: 1
    
    hustdbhavalue

Related fields:
  
* `Version` : version of the value.

**eturn value 1 (Value is matched, version is not matched):**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:06:06 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive
    Version1: 1
    Version2: 2
    
    hustdbhavalue

Related fields: 

* `Version1`: version of `master1`'s value. 
* `Version2`: version of `master2`'s value.

**Return value 3 (Value is not matched, version is matched):**

    HTTP/1.1 409 Conflict
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:07:01 GMT
    Content-Type: text/plain
    Content-Length: 26
    Connection: keep-alive
    Version: 1
    Val-Offset: 13
    
    hustdbhavalue0000000000000

Related fields:

* `Version`: version of the value.  
* `Val-Offset`: cut point of the two values. e.g. If we cut the value `hustdbhavalue0000000000000` at the offset of `13`, we get two sub values, `hustdbhavalue` and `0000000000000`, representing the values stored in `master1` and `master2` separately.

**Return value 4 (Both value and version are not matched):**

    HTTP/1.1 409 Conflict
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:19:04 GMT
    Content-Type: text/plain
    Content-Length: 26
    Connection: keep-alive
    Version1: 1
    Version2: 2
    Val-Offset: 13
    
    hustdbhavalue0000000000000

Related fields:

* `Version1`: version of `master1`'s value
* `Version2`:version of `master2`'s value
* `Val-Offset`:cut point of the two values. See more details in `Return value 3`.  

[Previous](../ha.md)

[Home](../../index.md)