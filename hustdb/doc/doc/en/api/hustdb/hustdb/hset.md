## hset ##

**Interface:** `/hustdb/hset`

**Method:** `GET | POST`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **val** (Required, GET：val is argument or POST：val is body)  
*  **ttl** (Optional, default：0)
*  **ver** (Optional, default：0)    

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/hset?tb=test_table&key=test_key&val=test_val"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not hash，reject hash request；or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/hset?tb=test_table&key=test_key&val=test_val&ver=2"

**Result B1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

[Previous page](../hustdb.md)

[Root directory](../../../index.md)