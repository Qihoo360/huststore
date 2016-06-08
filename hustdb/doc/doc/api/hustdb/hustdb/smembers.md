## smembers ##

**接口:** `/hustdb/smembers`

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

    curl -i -X GET "http://localhost:8085/hustdb/smembers?tb=test_table&offset=0&size=10"

**结果范例A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not set，reject set request；or tb not exist
	Version: 0
	VerError: false

**结果范例A2:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 688
	Content-Type: text/plain
	
	//key format: base64
	[{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzc1","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNA==","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNg==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzUx","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzE1","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzY3","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF83OA==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzMz","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF84","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF80OA==","ver":1}]

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/smembers?tb=test_table&offset=0&size=10&async=true"

**结果范例B1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 100 //when async is true, number of item in the snapshot 
	Content-Length: 688
	Content-Type: text/plain

	//key format: base64
	[{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzc1","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNA==","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNg==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzUx","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzE1","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzY3","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF83OA==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzMz","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF84","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF80OA==","ver":1}]

[上一级](../hustdb.md)

[根目录](../../../index.md)