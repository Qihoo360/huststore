## ack ##

**Interface:** `/ack`

**Method:** `GET`

**Parameter:** 

*  **queue** (Required) 
*  **peer** (Required)  参考 [get](get.md) 接口返回的 `Ack-Peer` 字段
*  **token** (Required) 参考 [get](get.md) 接口返回的 `Ack-Token` 字段

该接口是 `/hustmq/ack` 的代理接口，相关参数可参考 [这里](../hustmq/ack.md)。

**使用范例:**

请参考脚本 `hustmq/ha/nginx/test/autotest.py`

[Previous](../ha.md)

[Home](../../index.md)