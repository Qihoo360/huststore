## stat_all ##

**Interface:** `/hustdb/stat_all`

**Method:** `GET`

**Remark:**

	In totalize，return the storage profile of KV pairs and hashtable, excluding storage profiles of sort set, set and queue.
	The store.db.disk.storag_capacity configuration of hustdb.conf is to limit the storage capacity for kV pairs and hashtable, such as the capacity of kV pairs and hashtable is greater than store.db.disk.storage_capacity, and hustdb will prohibit insert operations.
	count: quantity; size: bytes; capacity: consumption of disk bytes.

**Sample A:**

    curl -i -X GET "http://localhost:8085/hustdb/stat_all"

**Result A1:**

	HTTP/1.1 200 OK
	Content-Length: 187
	Content-Type: text/plain

	[{"TOTALIZE":{"KVPAIR":{"count":2,"size":1500,"capacity":2048},"HASHTABLE":{"count":1,"size":750,"capacity":1024}}},{"table":"test_table","type":"H","count":1,"size":750,"capacity":1024}]

[Previous](../hustdb.md)

[Home](../../../index.md)