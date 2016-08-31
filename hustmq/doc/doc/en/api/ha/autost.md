## autost ##

**Interface:** `/autost`

**Method:** `GET`

**Parameter:**  N/A 

This interface is used to reflesh `hustmq` cluster status in real time, it will call [`/hustmq/stat_all`](../hustmq/stat_all.md) to query the real time status of all the backend `hustmq` servers, merge the result and save it to the local memory.

It's worthy to note that `hustmq ha` will periodically call [`/hustmq/stat_all`](../hustmq/stat_all.md) in the background to reflesh the cluster status, the default interval is `200` millisecond. Please see more details in [`autost_interval`](../../advanced/ha/nginx.md).

**Example:**

    curl -i -X GET "http://localhost:8080/autost"

**Animation:**
![autost](../../../res/autost.gif)

[Previous](../ha.md)

[Home](../../index.md)