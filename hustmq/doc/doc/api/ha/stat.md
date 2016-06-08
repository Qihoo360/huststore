## stat ##

**接口:** `/stat`

**方法:** `GET`

**参数:**  

*  **queue** （必选） 队列名称  

该接口用于获取指定队列的状态。需要说明的是，**接口返回的并非实时数据，而是缓存**（参考 [autost](autost.md)）。如果需要获取实时状态，可以在调用此接口之前先调用 [autost](autost.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8080/stat?queue=test_queue"

**返回值范例:**

接口返回的数据格式为 `json`，格式化之后的效果类似如下：

    {
        "queue":"hustmqhaqueue",
        "type":0,
        "ready":[0,0,0],
        "max":0,
        "lock":0,
        "si":1,
        "ci":1,
        "tm":1456822536
    }

相关的字段含义可参考 [`/hustmq/stat_all`](../hustmq/stat_all.md) 。

[上一级](../ha.md)

[根目录](../../index.md)