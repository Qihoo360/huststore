## exist ##

**Interface:** `/hustdb/exist`

**Method:** `GET`

**Parameter:** 

*  **key** (Required)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/exist?key=test_key"

**Result A1:**

	HTTP/1.1 404 Not Found
	Version: 0
	
**Result A2:**

	HTTP/1.1 200 OK
	Version: 1
	
[Previous page](../hustdb.md)

[Root directory](../../../index.md)