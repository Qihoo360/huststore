## smembers ##

**Interface:** `/hustdb/smembers`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)    
*  **offset** (Required)  
*  **size** (Required)
*  **start** (Optional, default: 0, >0 && <= 1024)  
*  **end** (Optional, default: 1024, >0 && <= 1024)
*  **noval** (Optional, default: true)
*  **async** (Optional, default: false, db dump;true, snapshot dump)  
Whether get key list from a snapshot, see more details in [export](export.md) 

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/smembers?tb=test_table&offset=0&size=10"

**Result A1:**

	HTTP/1.1 412 Precondition Failed //tb maybe not set, reject set request;or tb not exist
	Version: 0
	VerError: false

**Result A2:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 688
	Content-Type: text/plain
	
	//key format: base64
	[{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzc1","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNA==","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNg==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzUx","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzE1","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzY3","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF83OA==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzMz","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF84","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF80OA==","ver":1}]

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/smembers?tb=test_table&offset=0&size=10&async=true"

**R
Result B1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 100 //when async is true, number of item in the snapshot 
	Content-Length: 688
	Content-Type: text/plain

	//key format: base64
	[{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzc1","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNA==","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF8xNg==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzUx","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzE1","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzY3","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF83OA==","ver":1},{"key":"eHVydWlib3NldF9Sb2NrZXQsSHVzdERCLEh1c3RNUSxIdXN0TkdYXzMz","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF84","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF80OA==","ver":1}]

[Previous](../hustdb.md)

[Home](../../../index.md)
