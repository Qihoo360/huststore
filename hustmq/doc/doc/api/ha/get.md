`get`
----------

**接口:** `/get`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **worker** （必选）  
*  **ack** （可选）
  
该接口是 `/hustmq/get` 的代理接口，参数详情可参考 [这里](../hustmq/get.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/get?queue=test_queue&worker=test_worker"

**返回说明:**

当 `ack` 被设置为 `false` 时，该接口返回的 `http` 头部会包含如下两个字段：  

* `Ack-Peer`
* `Ack-Token`

这两个字段将会被 [ack 接口](ack.md) 所使用。

**动画效果:**
![get](get.gif)

[上一级](../ha.md)

[根目录](../../index.md)