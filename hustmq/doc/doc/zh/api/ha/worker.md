## worker ##

**接口:** `/worker`

**方法:** `GET`

**参数:**  

*  **queue** （必选）

该接口是 `/hustmq/worker` 的代理接口，参数详情可参考 [这里](../hustmq/worker.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/worker?queue=test_queue"

[上一页](../ha.md)

[回首页](../../index.md)