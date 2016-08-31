## keys ##

**Interface:** `/hustdb/keys`

**Method:** `GET`

**Parameter:** 

*  **file** (Required)  
represents db's id, check [file_count](file_count.md)  
*  **offset** (Required)  
*  **size** (Required)  
*  **start** (Optional, default: 0, >0 && <= 1024)  
*  **end** (Optional, default: 1024, >0 && <= 1024)  
*  **noval** (Optional, default: true)  
*  **async** (Optional, default: false, db dump: true, snapshot dump)  
Whether get key list from a snapshot, see more details in [export](export.md)  
  
**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/keys?file=0&offset=0&size=10"

**Result A1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 0 //when async is false, always be 0
	Content-Length: 535
	Content-Type: text/plain

	//key format: base64
	//tb: table name
    //ty: table type (H: hash, S: set, Z: sort set)
	[{"key":"a2V5XzcyX3hpb25nZmVpYWxs","ver":1},{"key":"Y2hlbmd6aHVvYWxsX2tleV8yNw==","ver":1},{"key":"a2V5XzRfZG9uZ3RpYW5jb25nYWxs","ver":1},{"key":"ZmFuYmluYWxsX2tleV80MQ==","ver":1},{"key":"a2V5XzcwX2NoZW5jaGFuZ3NodWFpYWxs","ver":1},{"key":"ZmFuYmluYWxsX2tleV8zMw==","ver":1},{"key":"a2V5Xzk2X3h1cnVpYm9hbGw=","ver":1},{"key":"a2V5XzU4X2Rvbmd0aWFuY29uZ2FsbA==","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF80OA==","ver":1,"tb":"test_table","ty":"S"},{"key":"a2V5XzQ0","val":"dmFsdWVfNDQ=","ver":1,"tb":"test_table","ty":"H"}]

**Sample B:**

    curl -i -X GET "http://localhost:8085/hustdb/keys?file=0&offset=0&size=10&async=true&noval=false"

**Result B1:**

	HTTP/1.1 200 OK
	Keys: 10 //number of item
	TotalCount: 271 //when async is true, number of item in the snapshot 
	Content-Length: 743
	Content-Type: text/plain

	//key format: base64
	//val format: base64
	//tb: table name
    //ty: table type (H: hash, S: set, Z: sort set)
	[{"key":"a2V5XzcyX3hpb25nZmVpYWxs","val":"dmFsdWVfNzI=","ver":1},{"key":"Y2hlbmd6aHVvYWxsX2tleV8yNw==","val":"Y2hlbmd6aHVvYWxsX3ZhbHVlXzI3","ver":1},{"key":"a2V5XzRfZG9uZ3RpYW5jb25nYWxs","val":"dmFsdWVfNA==","ver":1},{"key":"ZmFuYmluYWxsX2tleV80MQ==","val":"ZmFuYmluYWxsX3ZhbHVlXzQx","ver":1},{"key":"a2V5XzcwX2NoZW5jaGFuZ3NodWFpYWxs","val":"dmFsdWVfNzA=","ver":1},{"key":"ZmFuYmluYWxsX2tleV8zMw==","val":"ZmFuYmluYWxsX3ZhbHVlXzMz","ver":1},{"key":"a2V5Xzk2X3h1cnVpYm9hbGw=","val":"dmFsdWVfOTY=","ver":1},{"key":"a2V5XzU4X2Rvbmd0aWFuY29uZ2FsbA==","val":"dmFsdWVfNTg=","ver":1},{"key":"Um9ja2V0LEh1c3REQixIdXN0TVEsSHVzdE5HWF80OA==","ver":1,"tb":"test_table","ty":"S"},{"key":"a2V5XzQ0","val":"dmFsdWVfNDQ=","ver":1,"tb":"test_table","ty":"H"}]

[Previous](../hustdb.md)

[Home](../../../index.md)
