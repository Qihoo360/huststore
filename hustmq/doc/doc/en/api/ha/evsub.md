## evsub ##

**接口:** `/evsub`

**方法:** `GET`

**参数:** 

*  **queue** （必选）  
*  **idx** （必选）  推送数据的索引号
  
该接口和 [sub](sub.md)、[pub](pub.md) 相配合实现 `http push` 机制。其中 `idx` 会在第一次订阅的时候由服务端进行校验，如果不合法，服务端会返回 `403` 状态码，并且会将正确的索引号打包在 `http` 头部（字段名为 `Index`）。

详细的测试样例可以参考测试脚本。
脚本路径：`hustmq/ha/nginx/test/autotest.py`

**使用范例:**

    curl -i -X GET "http://localhost:8080/evsub?queue=test_queue&idx=1"

[上一级](../ha.md)

[根目录](../../index.md)