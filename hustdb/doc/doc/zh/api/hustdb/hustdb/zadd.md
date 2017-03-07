## zadd ##

**接口:** `/hustdb/zadd`

**方法:** `GET | POST`

**参数:** 

*  **tb** （必选）  
*  **score** （必选，>0）
*  **key** （必选，GET：key即参数 or POST：key即body）  
*  **opt** （可选，0,1,-1；default：0）
*  **ttl** （可选，default：0）
*  **ver** （可选，default：0）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=100&key=test_key"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not sort set，reject sort set request；or tb not exist
	Version: 0
	VerError: false

**结果范例A2:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false
    Content-Length: 3
	Content-Type: text/plain

    100

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=10&key=test_key&opt=1"

**结果范例B1:**

	HTTP/1.1 200 OK
	Version: 2
	VerError: false
    Content-Length: 3
	Content-Type: text/plain

    110 //if original value of test_key is 100, 100 + 10 = 110

**使用范例C:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=50&key=test_key&opt=-1"

**结果范例C1:**

	HTTP/1.1 200 OK
	Version: 3
	VerError: false
    Content-Length: 3
	Content-Type: text/plain

    110 //if original value of test_key is 110, 110 - 50 = 60

**使用范例D:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=101&key=test_key&ver=2"

**结果范例D1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

[上一页](../hustdb.md)

[回首页](../../../index.md)