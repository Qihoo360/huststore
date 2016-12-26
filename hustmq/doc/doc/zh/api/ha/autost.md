## autost ##

**接口:** `/autost`

**方法:** `GET`

**参数:**  无

该接口用于实时刷新 `hustmq` 集群状态，会调用 [`/hustmq/stat_all`](../hustmq/stat_all.md) 接口查询所有后端 `hustmq` 节点的实时状态，并对结果进行合并，保存到本地内存中。

需要说明的是，`hustmq ha` 会自动在后台周期性地调用 [`/hustmq/stat_all`](../hustmq/stat_all.md) 方法更新集群状态，默认的间隔是 `200` 毫秒，具体可参考 [`autost_interval`](../../advanced/ha/nginx.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/autost"

**动画效果:**
![autost](../../../res/autost.gif)

[上一级](../ha.md)

[回首页](../../index.md)