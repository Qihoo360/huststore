## hincrbyfloat ##

**接口:** `/cache/hincrbyfloat`

**方法:** `GET`

**参数:** 

*  **tb** （必选）  
*  **key** （必选）  
*  **val** （必选）  

该接口是 `/hustcache/hincrbyfloat` 的代理接口，参数详情可参考 [这里](../../hustdb/hustcache/hincrbyfloat.md) 。

**使用范例:**

    curl -i -X GET "http://localhost:8082/cache/hincrbyfloat?tb=test_table&key=test_key&val=5.9"

[上一级](../cache.md)

[回首页](../../../index.md)