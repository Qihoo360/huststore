## set_table ##

**Interface:** `/set_table`

**Method:** `POST`

**Parameter:** 

**Situation 1**  (Fetch `id`)

N/A

**Situation 2**  (Use fetcded `id` to set load balance table)

*  **id** (Required)  an legal id assigned by `hustdb ha`
*  **table** (Required)  new load balance table, must be put in `http body`. 

This interface is used to set load balance table at runtime. It could be used to update load balance table when `hustdb ha` cluster expands or shrinks in the amount of machine

Explanation: **This interface will restart HA so that the new config take effect.**. Because of this, security issue should be guaranteed when this interface is used. `hustdb ha` will dynamically generate an id and use `RSA` to encrypt it, so that it can be used as the legal identity. 

The working process of this interface is:    
- `client` sends `set_table` command to `hustdb ha` without any arugument, get an `RSA` encrypted id. 
- `client` gets the plain text id decrypted by private key.  
- `client` sends `set_table` command to `hustdb ha` (use plain text id as the argument, otherwise the requested will be denied), set load balance table.  
- `hustdb ha` checks legality of id and contents of the load balance table, if passed check, it will return http code `200` and update the current load balance table, restart the service.


**Sample:**

See more details in `hustdb/ha/nginx/test/set_table.py`

[Previous](../ha.md)

[Home](../../index.md)