## purge ##

**接口:** `/purge`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **priori** （必选）  

该接口是 `/hustmq/purge` 的代理接口，参数详情可参考 [这里](../hustmq/max.md)。

**使用范例:**

    curl -i -X GET "http://localhost:8080/purge?queue=test_queue&priori=0"

[上一页](../ha.md)

[回首页](../../index.md)