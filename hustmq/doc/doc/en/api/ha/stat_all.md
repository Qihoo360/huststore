## stat_all ##

**Interface:** `/stat_all`

**Method:** `GET`

**Parameter:**  N/A

This interface is used to query the status of `hustmq` cluster (cache), it's worthy to note that **the returned value is not real time data, but cached data.**(Check [autost](autost.md)).  If you want to get the real time status, you should call [autost](autost.md) before you call this interface. 

**Example:**

    curl -i -X GET "http://localhost:8080/stat_all"

**Return value:**

The data type of the returned data is `json`, it should look like this after formatted:

    [ 
        {
            "queue": "wise_command", 
            "type": 1, 
            "ready": [3,0,0], 
            "max": 0, 
            "lock": 0, 
            "si": 194, 
            "ci": 195, 
            "tm": 1456831841,
            "unacked":0,
            "timeout":5
        }, 
        {
            "queue": "Piece", 
            "type": 0, 
            "ready": [0, 0, 0], 
            "max": 0, 
            "lock": 0, 
            "si": 3, 
            "ci": 3, 
            "tm": 1456481392,
            "unacked":0,
            "timeout":5
        }
    ]

See more details in [`/hustmq/stat_all`](../hustmq/stat_all.md). 

**Animation:**
![stat_all](../../../res/stat_all.gif)

[Previous](../ha.md)

[Home](../../index.md)