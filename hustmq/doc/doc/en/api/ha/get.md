## get ##

**Interface:** `/get`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required)  
*  **worker** (Required)  
*  **ack** (Optional)
  
This is proxy interface for `/hustmq/get`, please refer to [Here](../hustmq/get.md).

**Example:**

    curl -i -X GET "http://localhost:8080/get?queue=test_queue&worker=test_worker"

**Return:**

when `ack` is `false`, this interface will return `http` header with the two following fields: 

* `Ack-Peer`
* `Ack-Token`

The two above fields will be used by [ack](ack.md).

**Animation:**
![get](../../../res/get.gif)

[Previous](../ha.md)

[Home](../../index.md)