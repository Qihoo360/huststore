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

    curl -i -X GET "http://localhost:8080/get?queue=test_queue"

**动画效果:**
![get](get.gif)

[上一级](../ha.md)

[根目录](../../index.md)