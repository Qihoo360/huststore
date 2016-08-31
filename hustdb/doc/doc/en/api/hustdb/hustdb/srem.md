## srem ##

**Interface:** `/hustdb/srem`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required, GET: key is argument or POST: key is body)  
*  **ver** (Optional, default: 0)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/srem?tb=test_table&key=test_key"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not set, reject set request;or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 404 Not Found
	Version: 0
	VerError: false
		
**Result A3:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/srem?tb=test_table&key=test_key&ver=2"

**Result B1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

**Result B2:**

	HTTP/1.1 200 OK
	Version: 2
	VerError: false

[Previous](../hustdb.md)

[Home](../../../index.md)