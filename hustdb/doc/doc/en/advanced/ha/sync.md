Data Synchronization Service
--

`sync server` is used for synchronization local log to the backend `db`, it includes two main components: `libsync` and `network`.

Note: **sync server is NOT necessity in deployment**. The following interfaces of HA will not work without `sync server`:

* [sync_status](../../api/ha/sync_status.md)
* [sync_alive](../../api/ha/sync_alive.md)

Besides, if the network between HA and DB is bad, **risk of inconsistency will increase**.

**Please make desicion by the production environment**.

### libsync ###

Directory: `hustdb/sync/module` 
#### `check_backend` ####

Description: Periodically detecting the aliveness of backend `db`s

if `db` is available, notify `read_log` module to read all log files of the `db` by `pipe` and start synchronizing.

#### `release_file` ####

Description: Periodically clean files when synchronization is completed.

Detect whether synchronization in files of `release_queue` is completed or not using bitmap, if completed, delete all files and release the resource.

#### `monitor` ####

Description: Monitor log directory

If log files are generated, add them into the queue of the corresponding `db`.

#### `read_log` ####

Description: Read log files

Get log files out from `file_queue`, read each file line by line and put the data to `data_queue` of thread pool.

After a file is read completely put it into `release_queue`.

#### `sync_threadpool` ####

Description: thread pool, synchronizing data

Each thread will fetch data from `data_queue`, decode `base64`, compose `url`, `query`, and then `POST` it to the backend `db`.

### network ###

Directory: `hustdb/sync/network`

This module provide `http` service **only for `HA` component**.

The two interfaces provided are:

#### `status` ####

**Interface:** `/status.html`

**Method:** `GET`

**Parameters:**  

This interface is used to check whether `sync server` is alive or not.  
Interface that are related to `HA`: [sync_alive](../../api/ha/sync_alive.md)

#### `sync_status` ####

**Interface:** `/sync_status`

**Method:** `GET`

**Parameters:** 

*  **backend_count** (required)  

This interface is used to fetch status from `sync server` when it's synchronizing data.  
Interface that is related to `HA`: [sync_status](../../api/ha/sync_status.md)

### Related Problems ###

**Q:**	How many threads that a `sync server` has?  
**A:**	 `4+n`, including `main thread`, `check_backend&release_file`, `monitor` and `read_log`, and n threads in thread pool. 

**Q:**  How do sub-modules of `sync server` cooperate with each other? 
**A:**  sub modules cooperate with each other through `pipe` or `queue`

**Q:**  How does `sync server` communicate with `HA`?  
**A:**  Use `libevhtp` as `http` framework to provide http service, `HA` uses sub request to query interface provided by `sync server`.

**Q:**  What are restrictions and cost of `sync server`?  
**A:**  For log generated before `sync server` started, `sync server` will not be able to automatically synchronize. If we want to synchronize these logs, we will have to copy them to the coressponding directory of `logs/` path manually.

[Previous](../ha.md)

[Home](../../index.md)






