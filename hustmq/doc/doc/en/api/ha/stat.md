## stat ##

**Interface:** `/stat`

**Method:** `GET`

**Parameter:**  

*  **queue** (Required) the name of queue 

This interface is used to fetch the status of the specific queue. **It's worthy to note that the return value is not real-time data, but cached data.** (Check [autost](autost.md)). If you want to get the real time status, you should call [autost](autost.md) before you call this interface. 

**Example:**

    curl -i -X GET "http://localhost:8080/stat?queue=test_queue"

**Return value:**

The data type of the returned data is `json`, it should look like this after formatted:

    {
        "queue":"hustmqhaqueue",
        "type":0,
        "ready":[0,0,0],
        "max":0,
        "lock":0,
        "si":1,
        "ci":1,
        "tm":1456822536
    }

See more details in [`/hustmq/stat_all`](../hustmq/stat_all.md). 

[Previous](../ha.md)

[Home](../../index.md)