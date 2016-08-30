## get_table ##

**Interface:** `/get_table`

**Method:** `GET`

**Parameter:** N/A

This interface is used to read load balance table.

**Sample:**

    curl -i -X GET "http://localhost:8082/get_table"

**Return value:**

	HTTP/1.1 200 OK
	Server: nginx/1.9.4
	Date: Mon, 29 Feb 2016 07:22:25 GMT
	Content-Type: text/plain
	Content-Length: 624
	Connection: keep-alive
	
	{
	    "table":
	    [
	        { "item": { "key": [0, 128], "val": ["10.120.100.178:9999", "10.120.100.178:9998"] } },
	        { "item": { "key": [128, 256], "val": ["10.120.100.178:9998", "10.120.100.179:9999"] } },
	        { "item": { "key": [256, 384], "val": ["10.120.100.179:9999", "10.120.100.179:9998"] } },
	        { "item": { "key": [384, 512], "val": ["10.120.100.179:9998", "10.120.100.180:9999"] } },
	        { "item": { "key": [512, 768], "val": ["10.120.100.180:9999", "10.120.100.180:9998"] } },
	        { "item": { "key": [768, 1024], "val": ["10.120.100.180:9998", "10.120.100.178:9999"] } }
	    ]
	}

[Previous](../ha.md)

[Home](../../index.md)