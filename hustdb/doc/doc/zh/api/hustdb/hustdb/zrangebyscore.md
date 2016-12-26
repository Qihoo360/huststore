## zrangebyscore ##

**接口:** `/hustdb/zrangebyscore`

**方法:** `GET`

**参数:** 

*  **tb** （必选）
*  **min** （必选）
*  **max** （必选）
*  **offset** （必选）
*  **size** （必选）
*  **start** （可选，default：0，>0 && <= 1024）  
*  **end** （可选，default：1024，>0 && <= 1024）
*  **noval** （可选，default：true）

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/zrangebyscore?tb=test_table&min=30&max=1987&offset=0&size=10&noval=false"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not sort set，reject sort set request；or tb not exist
	Version: 0
	VerError: false

**结果范例A2:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 256
	Content-Type: text/plain
	
	//key format: base64
	[{"key":"eHVydWlib196c2V0XzM=","val":"39","ver":1},{"key":"eHVydWlib196c2V0XzQ=","val":"87","ver":1},{"key":"eHVydWlib196c2V0XzU=","val":"345","ver":1},{"key":"eHVydWlib196c2V0XzY=","val":"569","ver":1},{"key":"eHVydWlib196c2V0Xzc=","val":"1986","ver":1}]

[上一页](../hustdb.md)

[回首页](../../../index.md)