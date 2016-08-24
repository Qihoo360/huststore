## sub ##

**接口:** `/sub`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **idx** （必选）  推送数据的索引号
  
该接口是 `/hustmq/sub` 的代理接口，参数详情可参考 [这里](../hustmq/sub.md)。。

**使用范例:**

    curl -i -X GET "http://localhost:8080/sub?queue=test_queue&idx=1"

[上一级](../ha.md)

[根目录](../../index.md)