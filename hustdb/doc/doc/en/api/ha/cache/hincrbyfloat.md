## hincrbyfloat ##

**Interface:** `/cache/hincrbyfloat`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)  
*  **key** (Required)  
*  **val** (Required)  

This interface is a proxy interface for `/hustcache/hincrbyfloat`. See more details in [here](../../hustdb/hustcache/hincrbyfloat.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/cache/hincrbyfloat?tb=test_table&key=test_key&val=5.9"

[Previous](../cache.md)

[Home](../../../index.md)