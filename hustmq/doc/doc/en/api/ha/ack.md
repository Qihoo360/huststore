## ack ##

**Interface:** `/ack`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required) 
*  **peer** (Required) Refer to `Ack-Peer` field returned by [get](get.md)
*  **token** (Required) Refer to `Ack-Token` field returned by [get](get.md)

This is proxy interface for `/hustmq/ack`, refer to [Here](../hustmq/ack.md).

**Example:**

Please refer to script `hustmq/ha/nginx/test/autotest.py`

[Previous](../ha.md)

[Home](../../index.md)