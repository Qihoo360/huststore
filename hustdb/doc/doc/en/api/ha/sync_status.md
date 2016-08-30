## sync_status ##

**Interface:** `/sync_status`

**Method:** `GET`

**Parameter:** 

This interface is used to get sync status info of `sync server`.

**Sample:**

    curl -i -X GET "http://localhost:8082/sync_status"

**Return value:**

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
[Previous](../ha.md)

[Home](../../index.md)