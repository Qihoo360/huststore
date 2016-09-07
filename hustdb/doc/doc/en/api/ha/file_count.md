## file_count ##

**Interface:** `/file_count`

**Method:** `GET`

**Parameter:** 

*  **peer** (Required)  
This represents the indexes of backend server. See more details in [peer_count](peer_count.md)  

This Interface is a proxy interface for `/hustdb/file_count`. See more details in [here](../hustdb/hustdb/file_count.md).  

**Sample:**

    curl -i -X GET "http://localhost:8082/file_count?peer=0"

[Previous](../ha.md)

[Home](../../index.md)