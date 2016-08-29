## zadd ##

**Interface:** `/hustdb/zadd`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **score** (Required)
*  **key** (Required, GET：key is argument or POST：key is body)   
*  **opt** (Optional, 0,1,-1；default：0)
*  **ttl** (Optional, default：0)
*  **ver** (Optional, default：0)

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=100&key=test_key"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not sort set，reject sort set request；or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/zadd?tb=test_table&score=101&key=test_key&ver=2"

**Result B1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

[Previous page](../hustdb.md)

[Home](../../../index.md)