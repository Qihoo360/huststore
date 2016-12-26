## hkeys ##

**接口:** `/hustdb/hkeys`

**方法:** `GET`

**参数:** 

*  **tb** （必选）    
*  **offset** （必选）  
*  **size** （必选）
*  **start** （可选，default：0，>0 && <= 1024）  
*  **end** （可选，default：1024，>0 && <= 1024）
*  **noval** （可选，default：true）
*  **async** （可选，default：false，db dump；true，snapshot dump）  
表示是否通过快照获取key列表，参考 [export](export.md)
  
**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/hkeys?tb=test_table&offset=0&size=10"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not hash，reject hash request；or tb not exist
	Version: 0
	VerError: false

**结果范例A2:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 583
	Content-Type: text/plain

    //key format: base64
	[{"key":"eHVydWlib19rZXlfOTM=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfNTM=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfMTc=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfNjE=","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5XzU0","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfOTE=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfNQ==","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5XzUy","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5Xzg2","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5XzQ0","ver":1,"tb":"test_table","ty":"H"}]

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/hkeys?tb=test_table&offset=0&size=10&noval=false"

**结果范例B1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 622
	Content-Type: text/plain

	//key format: base64
	//val format: base64
	[{"key":"eHVydWlib19rZXlfOTM=","val":"eHVydWlib192YWx1ZV85Mw==","ver":1},{"key":"eHVydWlib19rZXlfNTM=","val":"eHVydWlib192YWx1ZV81Mw==","ver":1},{"key":"eHVydWlib19rZXlfMTc=","val":"eHVydWlib192YWx1ZV8xNw==","ver":1},{"key":"eHVydWlib19rZXlfNjE=","val":"eHVydWlib192YWx1ZV82MQ==","ver":1},{"key":"a2V5XzU0","val":"dmFsdWVfNTQ=","ver":1},{"key":"eHVydWlib19rZXlfOTE=","val":"eHVydWlib192YWx1ZV85MQ==","ver":1},{"key":"eHVydWlib19rZXlfNQ==","val":"eHVydWlib192YWx1ZV81","ver":1},{"key":"a2V5XzUy","val":"dmFsdWVfNTI=","ver":1},{"key":"a2V5Xzg2","val":"dmFsdWVfODY=","ver":1},{"key":"a2V5XzQ0","val":"dmFsdWVfNDQ=","ver":1}]

**使用范例C:**

    curl -i -X GET "http://localhost:8085/hustdb/hkeys?tb=test_table&offset=0&size=10&async=true"

**结果范例C1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 100 //when async is true, number of item in the snapshot 
	Content-Length: 344
	Content-Type: text/plain

	//key format: base64
	//tb: table name
    //ty: table type (H: hash, S: set, Z: sort set)
	[{"key":"eHVydWlib19rZXlfOTM=","ver":1},{"key":"eHVydWlib19rZXlfNTM=","ver":1},{"key":"eHVydWlib19rZXlfMTc=","ver":1},{"key":"eHVydWlib19rZXlfNjE=","ver":1},{"key":"a2V5XzU0","ver":1},{"key":"eHVydWlib19rZXlfOTE=","ver":1},{"key":"eHVydWlib19rZXlfNQ==","ver":1},{"key":"a2V5XzUy","ver":1},{"key":"a2V5Xzg2","ver":1},{"key":"a2V5XzQ0","ver":1}]

[上一页](../hustdb.md)

[回首页](../../../index.md)