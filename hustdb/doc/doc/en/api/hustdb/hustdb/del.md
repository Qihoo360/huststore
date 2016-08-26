## del ##

**Interface:** `/hustdb/del`

**Method:** `GET`

**Parameter:** 

*  **key** (Required)  
*  **ver** (Optional, default：0)  


**SampleA:**

    curl -i -X GET "http://localhost:8085/hustdb/del?key=test_key"

**ResultA1:**

	HTTP/1.1 404 Not Found
	Version: 0
	VerError: false
		
**ResultA2:**

	HTTP/1.1 200 OK
	Version: 1
	VerError: false

**SampleB:**

    curl -i -X GET "http://localhost:8085/hustdb/del?key=test_key&ver=2"

**ResultB1:**

	HTTP/1.1 401 Unauthorized
	Version: 1
	VerError: true

**ResultB2:**

	HTTP/1.1 200 OK
	Version: 2
	VerError: false

[Previous page](../hustdb.md)

[Root directory](../../../index.md)