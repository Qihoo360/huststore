## hincrby ##

**Interface:** `/hustdb/hincrby`

**Method:** `GET`

**Parameter:** 

*  **tb** (必选)  
*  **key** (必选)  
*  **val** (必选)  
*  **ttl** (可选, default: 0)
*  **ver** (可选, default: 0) 

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/hset?tb=test_table&key=test_key&val=7"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not hash, reject hash request;or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false
    Content-Length: 2
	Content-Type: text/plain

	16 //if original value of test_key is 9, 9 + 7 = 16

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/hset?tb=test_table&key=test_key&val=7&ver=2"

**Result B1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

[Previous](../hustcache.md)

[Home](../../../index.md)