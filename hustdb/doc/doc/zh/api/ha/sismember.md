## sismember ##

**接口:** `/sismember`

**方法:** `POST`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
该参数必须放在 `http body` 中

该接口是 `/hustdb/sismember` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/sismember.md) 。

**使用范例:**

    curl -i -X POST "http://localhost:8082/sismember?tb=test_table" -d "test_key"

[上一页](../ha.md)

[回首页](../../index.md)