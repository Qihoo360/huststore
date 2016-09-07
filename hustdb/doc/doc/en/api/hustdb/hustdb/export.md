## export ##

**Interface:** `/hustdb/export`

**Method:** `GET`

**Parameter:** 

*  **file** (Required, choose one between `file` and `tb`)
*  **tb** (Required, choose one between `file` and `tb`)
*  **start** (Optional, default：0,>0 && <= 1024)  
*  **end** (Optional, default：1024, >0 && <= 1024)
*  **noval** (Optional, default：true)
*  **cover** (Optional, default：false)

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/export?file=0"

**Result A1:**

	HTTP/1.1 200 OK
	Content-Length: 15
	Content-Type: text/plain

	140002579198016 //task token

**Result A2:**

	HTTP/1.1 404 Not Found //If file0 has already done, you need to use cover

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/export?file=0&cover=true"

**Result B1:**

	HTTP/1.1 200 OK
	Content-Length: 15
	Content-Type: text/plain

	140002914742624 //task token

[Previous](../hustdb.md)

[Home](../../../index.md)
