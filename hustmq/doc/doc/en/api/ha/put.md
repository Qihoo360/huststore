## put ##

**Interface:** `/put`

**Method:** `GET | PUT | POST`

**Parameter:**  

*  **queue** (Required)
*  **item** (Required)
*  **priori** (Optional)

This is the proxy interface of `/hustmq/put`, see more details in [here](../hustmq/put.md).

**Example:**

    curl -i -X POST "http://localhost:8080/put?queue=test_queue" -d "test_item"

**Animation:**
![put](../../../res/put.gif)

[Previous](../ha.md)

[Home](../../index.md)