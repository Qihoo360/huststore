## peer_count ##

**Interface:** `/peer_count`

**Method:** `GET`

**Parameter:** 

This interface is used to get the node count of the backend `hustdb`. Interfaces that depend on this `API` are:   

* [stat](stat.md)
* [stat_all](stat_all.md)
* [keys](keys.md)
* [hkeys](hkeys.md)
* [smembers](smembers.md)
* [export](export.md)
* [file_count](file_count.md)

The above interfaces need to pass `peer` argument. If `peer_count`'s value is **N**, then its value range will be in **[0, N-1]**. 

**Sample:**

    curl -i -X GET "http://localhost:8082/peer_count"

[Previous](../ha.md)

[Home](../../index.md)