## smembers ##

**接口:** `/smembers`

**方法:** `GET`

**参数:** 

*  **peer** （必选）  
表示后端机节点索引，参考 [peer_count](peer_count.md)  
*  **tb** （必选）
*  **offset** （必选）  
*  **size** （必选）  
*  **start** （可选）  
*  **end** （可选）    
*  **noval** （可选）   
*  **async** （可选）    

该接口是 `/hustdb/smembers` 的代理接口，参数详情可参考 [这里](../hustdb/hustdb/smembers.md) 。

**使用范例:**

参考测试脚本的写法。  
脚本路径： `hustdb/ha/nginx/test/fetch.py`

[上一级](../ha.md)

[根目录](../../index.md)