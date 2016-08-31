## zismember ##

**Interface:** `/hustdb/zismember`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required, GET: key is argument or POST: key is body)   

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/zismember?tb=test_table&key=test_key"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not sort set, reject sort set request;or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 404 Not Found
	Version: 0
	
**Result A3:**

	HTTP/1.1 200 OK
	Version: 1

[Previous](../hustdb.md)

[Home](../../../index.md)