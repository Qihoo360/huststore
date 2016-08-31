## zrangebyscore ##

**Interface:** `/zrangebyscore`

**Method:** `GET`

**Parameter:** 

*  **tb** (Required)
*  **min** (Required)
*  **max** (Required)
*  **offset** (Required)  
*  **size** (Required)  
*  **start** (Optional)  
*  **end** (Optional)    
*  **noval** (Optional)   

This interface is a proxy interface for `/hustdb/zrangebyscore`. See more details in [here](../hustdb/hustdb/zrangebyscore.md).  

**Sample:**

See more details in the example test script:  
Script path: `hustdb/ha/nginx/test/fetch.py`

[Previous](../ha.md)

[Home](../../index.md)