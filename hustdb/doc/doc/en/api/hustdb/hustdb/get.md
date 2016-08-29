## get ##

**Interface:** `/hustdb/get`

**Method:** `GET`

**Parameter:** 

*  **key** (Required)

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/get?key=test_key"

**Result A1:**

	HTTP/1.1 404 Not Found
	Version: 0

**Result A2:**

	HTTP/1.1 200 OK
	Version: 1
	Content-Length: 8
	Content-Type: text/plain

	test_val

[Previous page](../hustdb.md)

[Home](../../../index.md)