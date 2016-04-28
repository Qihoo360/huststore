`sync_status`
----------

**接口:** `/sync_status`

**方法:** `GET`

**参数:** 无

该接口用于获取 `libsync` 模块进行数据同步的实时状态。

**使用范例:**

    curl -i -X GET "http://localhost:8082/sync_status"

**返回值范例:**

	{
	   "status" : {
	      "10.120.100.178:9998" : {
	         "processed_items" : 0,
	         "synced_items" : 0
	      },
	      "10.120.100.178:9999" : {
	         "processed_items" : 0,
	         "synced_items" : 0
	      },
	      "10.120.100.179:9998" : {
	         "processed_items" : 0,
	         "synced_items" : 0
	      },
	      "10.120.100.179:9999" : {
	         "processed_items" : 0,
	         "synced_items" : 0
	      },
	      "10.120.100.180:9998" : {
	         "processed_items" : 0,
	         "synced_items" : 0
	      },
	      "10.120.100.180:9999" : {
	         "processed_items" : 0,
	         "synced_items" : 0
	      }
	   },
	   "total_status" : {
	      "processed_items" : 0,
	      "synced_items" : 0
	   }
	}
[上一级](../ha.md)

[根目录](../../index.md)