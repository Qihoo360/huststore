`zrangebyrank`
----------

**接口:** `/hustdb/zrangebyrank`

**方法:** `GET`

**参数:** 

*  **tb** （必选）
*  **offset** （必选）
*  **size** （必选）
*  **start** （可选，default：0，>0 && <= 1024）  
*  **end** （可选，default：1204，>0 && <= 1024）
*  **noval** （可选，default：true）
*  **async** （可选，default：false，db dump；true，snapshot dump）  
表示是否通过快照获取key列表，参考 [export](export.md)

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/zrangebyrank?tb=test_table&offset=0&size=10"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not sort set，reject sort set request；or tb not exist
	Version: 0
	VerError: false

**结果范例A2:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 512
	Content-Type: text/plain
	
	//key format: base64
	[{"key":"eHVydWlib196c2V0XzE=","val":"9","ver":1},{"key":"eHVydWlib196c2V0XzI=","val":"13","ver":1},{"key":"eHVydWlib196c2V0XzM=","val":"39","ver":1},{"key":"eHVydWlib196c2V0XzQ=","val":"87","ver":1},{"key":"eHVydWlib196c2V0XzU=","val":"345","ver":1},{"key":"eHVydWlib196c2V0XzY=","val":"569","ver":1},{"key":"eHVydWlib196c2V0Xzc=","val":"1986","ver":1},{"key":"eHVydWlib196c2V0Xzg=","val":"4756","ver":1},{"key":"eHVydWlib196c2V0Xzk=","val":"7654","ver":1},{"key":"eHVydWlib196c2V0XzEw","val":"14567","ver":1}]

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/zrangebyrank?tb=test_table&offset=0&size=10&async=true"

**结果范例B1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 100 //when async is true, number of item in the snapshot 
	Content-Length: 512
	Content-Type: text/plain

	//key format: base64
	[{"key":"eHVydWlib196c2V0XzE=","val":"9","ver":1},{"key":"eHVydWlib196c2V0XzI=","val":"13","ver":1},{"key":"eHVydWlib196c2V0XzM=","val":"39","ver":1},{"key":"eHVydWlib196c2V0XzQ=","val":"87","ver":1},{"key":"eHVydWlib196c2V0XzU=","val":"345","ver":1},{"key":"eHVydWlib196c2V0XzY=","val":"569","ver":1},{"key":"eHVydWlib196c2V0Xzc=","val":"1986","ver":1},{"key":"eHVydWlib196c2V0Xzg=","val":"4756","ver":1},{"key":"eHVydWlib196c2V0Xzk=","val":"7654","ver":1},{"key":"eHVydWlib196c2V0XzEw","val":"14567","ver":1}]

[上一级](../hustdb.md)

[根目录](../../../index.md)