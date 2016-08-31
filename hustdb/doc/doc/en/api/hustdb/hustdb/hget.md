## hget ##

**Interface:** `/hustdb/hget`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/hget?tb=test_table&key=test_key"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not hash, reject hash request；or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 404 Not Found
	Version: 0

**Result A3:**

	HTTP/1.1 200 OK
	Version: 1
	Content-Length: 8
	Content-Type: text/plain

	test_val

[Previous](../hustdb.md)

[Home](../../../index.md)