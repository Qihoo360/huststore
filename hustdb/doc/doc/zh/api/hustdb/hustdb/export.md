## export ##

**接口:** `/hustdb/export`

**方法:** `GET`

**参数:** 

*  **file** （必选，与tb二选一）
*  **tb** （必选，与file二选一）  
*  **start** （可选，default：0，>0 && <= 1024）  
*  **end** （可选，default：1024，>0 && <= 1024）
*  **noval** （可选，default：true）
*  **cover** （可选，default：false）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/export?file=0"

**结果范例A1:**

	HTTP/1.1 200 OK
	Content-Length: 15
	Content-Type: text/plain

	140002579198016 //task token

**结果范例A2:**

	HTTP/1.1 404 Not Found //If file0 has already done, you need to use cover

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/export?file=0&cover=true"

**结果范例B1:**

	HTTP/1.1 200 OK
	Content-Length: 15
	Content-Type: text/plain

	140002914742624 //task token

[上一级](../hustdb.md)

[回首页](../../../index.md)