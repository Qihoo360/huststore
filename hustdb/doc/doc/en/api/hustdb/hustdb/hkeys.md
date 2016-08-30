## hkeys ##

**Interface:** `/hustdb/hkeys`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)    
*  **offset** (Required)  
*  **size** (Required)  
*  **start** (Optional, default：0，>0 && <= 1024)  
*  **end** (Optional, default：1204，>0 && <= 1024)
*  **noval** (Optional, default：true)
*  **async** (Optional, default：false，db dump；true，snapshot dump)  
Whether get key list from snapshot, please refer to [export](export.md)
  
**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/hkeys?tb=test_table&offset=0&size=10"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not hash，reject hash request；or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 583
	Content-Type: text/plain

    //key format: base64
	[{"key":"eHVydWlib19rZXlfOTM=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfNTM=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfMTc=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfNjE=","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5XzU0","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfOTE=","ver":1,"tb":"test_table","ty":"H"},{"key":"eHVydWlib19rZXlfNQ==","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5XzUy","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5Xzg2","ver":1,"tb":"test_table","ty":"H"},{"key":"a2V5XzQ0","ver":1,"tb":"test_table","ty":"H"}]

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/hkeys?tb=test_table&offset=0&size=10&noval=false"

**Result B1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 622
	Content-Type: text/plain

	//key format: base64
	//val format: base64
	[{"key":"eHVydWlib19rZXlfOTM=","val":"eHVydWlib192YWx1ZV85Mw==","ver":1},{"key":"eHVydWlib19rZXlfNTM=","val":"eHVydWlib192YWx1ZV81Mw==","ver":1},{"key":"eHVydWlib19rZXlfMTc=","val":"eHVydWlib192YWx1ZV8xNw==","ver":1},{"key":"eHVydWlib19rZXlfNjE=","val":"eHVydWlib192YWx1ZV82MQ==","ver":1},{"key":"a2V5XzU0","val":"dmFsdWVfNTQ=","ver":1},{"key":"eHVydWlib19rZXlfOTE=","val":"eHVydWlib192YWx1ZV85MQ==","ver":1},{"key":"eHVydWlib19rZXlfNQ==","val":"eHVydWlib192YWx1ZV81","ver":1},{"key":"a2V5XzUy","val":"dmFsdWVfNTI=","ver":1},{"key":"a2V5Xzg2","val":"dmFsdWVfODY=","ver":1},{"key":"a2V5XzQ0","val":"dmFsdWVfNDQ=","ver":1}]

**Sample C:**

    curl -i -X GET "http://localhost:8085/hustdb/hkeys?tb=test_table&offset=0&size=10&async=true"

**Result C1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 100 //when async is true, number of item in the snapshot 
	Content-Length: 344
	Content-Type: text/plain

	//key format: base64
	//tb: table name
    //ty: table type (H: hash, S: set, Z: sort set)
	[{"key":"eHVydWlib19rZXlfOTM=","ver":1},{"key":"eHVydWlib19rZXlfNTM=","ver":1},{"key":"eHVydWlib19rZXlfMTc=","ver":1},{"key":"eHVydWlib19rZXlfNjE=","ver":1},{"key":"a2V5XzU0","ver":1},{"key":"eHVydWlib19rZXlfOTE=","ver":1},{"key":"eHVydWlib19rZXlfNQ==","ver":1},{"key":"a2V5XzUy","ver":1},{"key":"a2V5Xzg2","ver":1},{"key":"a2V5XzQ0","ver":1}]

[Previous](../hustdb.md)

[Home](../../../index.md)