## get2 ##

**接口:** `/get2`

**方法:** `GET`

**参数:** 

*  **key** （必选）  

该接口是 `/hustdb/get` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/get.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/get2?key=test_key"

根据数据的版本以及值的一致性的情况，该接口的返回一共有如下几种：

**返回范例1（值一致，版本一致）:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:05:31 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive
    Version: 1
    
    hustdbhavalue

相关字段：
  
* `Version` ：值的版本号。

**返回范例2（值一致，版本不一致）:**

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:06:06 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive
    Version1: 1
    Version2: 2
    
    hustdbhavalue

相关字段：

* `Version1`：`master1` 的值的版本号。  
* `Version2`：`master2` 的值的版本号。

**返回范例3（值不一致，版本一致）:**

    HTTP/1.1 409 Conflict
    Server: nginx/1.9.4
    Date: Thu, 19 May 2016 10:07:01 GMT
    Content-Type: text/plain
    Content-Length: 26
    Connection: keep-alive
    Version: 1
    Val-Offset: 13
    
    hustdbhavalue0000000000000

相关字段：

* `Version`：版本号。  
* `Val-Offset`：两个值的分割点。可以看到 `hustdbhavalue0000000000000` 以偏移为 `13` 的地方分开，可以得到两个子串，分别为 `hustdbhavalue` 和 `0000000000000`，分别代表 `master1` 和 `master2` 存放的值。

**返回范例4（值不一致，版本不一致）:**

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

相关字段：

* `Version1`：`master1` 的值的版本号。  
* `Version2`：`master2` 的值的版本号。
* `Val-Offset`：两个值的分割点。参考 `返回范例3` 的解释。

[上一级](../ha.md)

[根目录](../../index.md)