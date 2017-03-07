## zadd ##

**Interface:** `/hustdb/zadd`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **score** (Required, >0)
*  **key** (Required, GET: key is argument or POST: key is body)   
*  **opt** (Optional, 0,1,-1;default: 0)
*  **ttl** (Optional, default: 0)
*  **ver** (Optional, default: 0)

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=100&key=test_key"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not sort set, reject sort set request;or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

    100

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=10&key=test_key&opt=1"

**Result B1:**

	HTTP/1.1 200 OK
	Version: 2
	VerError: false
    Content-Length: 3
	Content-Type: text/plain

    110 //if original value of test_key is 100, 100 + 10 = 110

**Sample C:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=50&key=test_key&opt=-1"

**Result C1:**

	HTTP/1.1 200 OK
	Version: 3
	VerError: false
    Content-Length: 3
	Content-Type: text/plain

    110 //if original value of test_key is 110, 110 - 50 = 60

**Sample D:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=101&key=test_key&ver=2"

**Result D1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

[Previous](../hustdb.md)

[Home](../../../index.md)