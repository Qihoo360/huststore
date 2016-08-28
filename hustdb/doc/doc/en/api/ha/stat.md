## stat ##

**Interface:** `/stat`

**Method:** `GET`

**Parameter:** 

*  **peer** (Required)  
This represents the indexes of backend servers. See more details in [peer_count](peer_count.md)
*  **tb** (Optional)  

This Interface is an proxy interface for `/hustdb/stat`. See more details in [here](../hustdb/hustdb/stat.md).  

**Sample:**

It's not quite trivial to write script for this interface, please refer to more details in the example test script:   
Script path: `hustdb/ha/nginx/test/stat.py`

[Previous page](../ha.md)

[Root directory](../../index.md)