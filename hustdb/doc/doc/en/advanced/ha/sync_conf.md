Sync Server Configuration
--

### Service Configuration ###
  
Source code path: `hustdb/sync/module/sync_server.json`  
Installation path: `/data/hustdbsync/sync_server.json`

A complete example of configuration file: 

    {
        "network": 
        {
            "port": 8089,
            "backlog": 512,
            "disable_100_cont": true,
            "enable_reuseport": true,
            "enable_nodelay": true,
            "enable_defer_accept": true,
            "max_body_size": 16777216,
            "max_keepalive_requests": 1024,
            "recv_timeout": 300,
            "send_timeout": 300,
            "threads": 2,
            "user": "sync",
            "passwd": "sync",
            "access_allow": "127.0.0.1"
        },
        "sync":
        {
            "daemon": true,
            "logs_path": "/data/hustdbha/logs/",
            "ngx_path":  "/data/hustdbha/",
            "auth_path": "/data/hustdbha/conf/htpasswd",
            "threads": 4,
            "release_interval": 5000,
            "checkdb_interval": 5000,
            "checklog_interval": 60000
        }
    }

**Keep default value for the below fields**

* `network`
    * `access_allow`: access restriction list for `sync server`

**Keep settings the of below fields same as that in HA configuration file**

* `network`
    * `port`: Listening port of `sync server`
    * `user`: Username for `sync_server` to do `http basic authentication`
    * `passwd`: Password for `sync_server` to do `http basic authentication`
* `sync`
    * `logs_path`: Path for `HA` output log, this log is use as the input for `sync server` to synchronize data
    * `ngx_path`: Path for `HA` deployment, `sync server` will store synchronization status files in this directory
    * `auth_path`: Path for files that store username and password for `HA` to access `hustdb`, will be used as arguments of `http` request by `sync server`

**Set a suitable value for the below field in your specific network environment**

* `sync`
    * `threads`: Number of threads for data synchronization
    * `release_interval`: Time interval for detecting the completion status of log file synchronization. If synchronization is completed, the log file will be deleted
    * `checkdb_interval`: Time interval for detecting aliveness of backend `db`, only when backend `db` is alived synchronization will begin
    * `checklog_interval`: Time interval for detecting whether the write operations have been completed in log file, this value is the same as that of `zlog` generating log file

**In exception of this, use default values for other fields.**

### Log Configuration ###
  
Source code path: `hustdb/sync/module/zlog.conf`  
Installation path: `data/hustdbsync/zlog.conf`

A complete example of configuration file: 

    [global]
    strict init = true
    buffer min = 2MB
    buffer max = 64MB
    rotate lock file = /tmp/zlog_hustdbsync.lock
    file perms = 755
    
    [formats]
    default = "[%d] [%V] | %m%n"
    
    [rules]
    hustdbsync.*             "/data/hustdbsync/logs/%d(%Y-%m-%d).log"; default

If the deployment path of `sync server` is not `/data/hustdbsync`, then its `[rules]` should be changed to the corresponding path.


[Previous](conf.md)

[Home](../../index.md)