## sadd ##

**接口:** `/sadd`

**方法:** `POST`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
该参数必须放在 `http body` 中
*  **ver** （可选）

该接口是 `/hustdb/sadd` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/sadd.md) 。

**使用范例:**

    curl -i -X POST "http://localhost:8082/sadd?tb=test_table" -d "test_key"

[上一级](../ha.md)

[回首页](../../index.md)