## stat ##

**Interface:** `/hustdb/stat`

**Method:** `GET`

**Parameter:** 

*  **tb** (Optional: return number of KV pairs, or return number of KV pairs of corresponding table)  

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/stat"

**Result A1:**

	HTTP/1.1 200 OK
	Content-Length: 1
	Content-Type: text/plain

	1

[Previous page](../hustdb.md)

[Home](../../../index.md)