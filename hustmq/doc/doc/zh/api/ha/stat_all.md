## stat_all ##

**接口:** `/stat_all`

**方法:** `GET`

**参数:**  无

该接口用于获取 `hustmq` 集群状态（缓存）。需要说明的是，**接口返回的并非实时数据，而是缓存**（参考 [autost](autost.md)）。如果需要获取实时状态，可以在调用此接口之前先调用 [autost](autost.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8080/stat_all"

**返回值范例:**

接口返回的数据格式为 `json`，格式化之后的效果类似如下：

    [ 
        {
            "queue": "wise_command", 
            "type": 1, 
            "ready": [3,0,0], 
            "max": 0, 
            "lock": 0, 
            "si": 194, 
            "ci": 195, 
            "tm": 1456831841,
            "unacked":0,
            "timeout":5
        }, 
        {
            "queue": "Piece", 
            "type": 0, 
            "ready": [0, 0, 0], 
            "max": 0, 
            "lock": 0, 
            "si": 3, 
            "ci": 3, 
            "tm": 1456481392,
            "unacked":0,
            "timeout":5
        }
    ]

相关的字段含义可参考 [`/hustmq/stat_all`](../hustmq/stat_all.md) 。

**动画效果:**
![stat_all](../../../res/stat_all.gif)

[上一页](../ha.md)

[回首页](../../index.md)