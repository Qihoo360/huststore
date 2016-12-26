## max ##

**接口:** `/max`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **num** （必选）  

该接口是 `/hustmq/max` 的代理接口，参数详情可参考 [这里](../hustmq/max.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/max?queue=test_queue&num=1000"

[上一级](../ha.md)

[回首页](../../index.md)