`stat`
----------

**接口:** `/stat`

**方法:** `GET`

**参数:** 

*  **peer** （必选）  
表示后端机节点索引，参考 [peer_count](peer_count.md)
*  **tb** （可选）  

该接口是 `/hustdb/stat` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/stat.md) 。

**使用范例:**

该接口的测试代码的写法相对复杂，可参考测试脚本的写法。  
脚本路径： `hustdb/ha/nginx/test/stat.py`

[上一级](../ha.md)

[根目录](../../index.md)