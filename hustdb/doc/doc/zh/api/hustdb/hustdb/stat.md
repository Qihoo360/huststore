## stat ##

**接口:** `/hustdb/stat`

**方法:** `GET`

**参数:** 

*  **tb** （可选；返回kv键值存储概况及hashtable存储概况，或者，返回对应table的存储概况）

**备注:**

	TOTALIZE中，返回kv键值存储概况及hashtable存储概况，不包括sort set、set、queue的存储概况。
	hustdb.conf的store.db.disk.storag_capacity配置即针对kv键值及hashtable做存储容量限制，如kv键值及hashtable的capacity之和大于store.db.disk.storage_capacity，hustdb将禁止写入操作。
	count：数量；size：字节数；capacity：消耗磁盘字节数。

**使用范例A:**

    curl -i -X GET "http://localhost:8085/hustdb/stat"

**结果范例A1:**

	HTTP/1.1 200 OK
	Content-Length: 114
	Content-Type: text/plain

	{"TOTALIZE":{"KVPAIR":{"count":2,"size":1500,"capacity":2048},"HASHTABLE":{"count":1,"size":750,"capacity":1024}}}

**使用范例B:**

    curl -i -X GET "http://localhost:8085/hustdb/stat?tb=test_table"

**结果范例B1:**

	HTTP/1.1 200 OK
	Content-Length: 70
	Content-Type: text/plain

	{"table":"test_table","type":"H","count":1,"size":750,"capacity":1024}

[上一页](../hustdb.md)

[回首页](../../../index.md)