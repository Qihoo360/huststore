`put`
----------

**接口:** `/put`

**方法:** `GET | PUT | POST`

**参数:**  

*  **queue** （必选）
*  **item** （必选）
*  **priori** （可选）

该接口是 `/hustmq/put` 的代理接口，参数详情可参考 [这里](../hustmq/put.md)。

**使用范例:**

    curl -i -X POST "http://localhost:8080/put?queue=test_queue" -d "test_item"

**动画效果:**
![put](put.gif)

[上一级](../ha.md)

[根目录](../../index.md)