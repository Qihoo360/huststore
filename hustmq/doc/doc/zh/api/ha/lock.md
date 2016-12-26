## lock ##

**接口:** `/lock`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **on** （必选）  

该接口是 `/hustmq/lock` 的代理接口，参数详情可参考 [这里](../hustmq/lock.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/lock?queue=test_queue&on=1"

**动画效果:**
![lock](../../../res/lock.gif)

[上一级](../ha.md)

[回首页](../../index.md)