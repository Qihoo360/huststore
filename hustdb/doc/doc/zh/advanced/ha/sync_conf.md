sync server 配置文件
--

### 服务配置 ###
  
源码路径：`hustdb/sync/module/sync_server.json`  
安装路径：`/data/hustdbsync/sync_server.json`

以下是一个完整的配置文件：

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

**以下字段请保持默认值：**

* `network`
    * `access_allow`: `sync server` 的 `ip` 访问限制列表

**以下字段请和 HA 的相关配置保持一致：**

* `network`
    * `port`: `sync server` 的监听端口
    * `user`: `sync_server` 进行 `http basic authentication` 的用户名
    * `passwd`: `sync_server` 进行 `http basic authentication` 的密码
* `sync`
    * `logs_path`: `HA` 输出日志的目录，该目录是 `sync server` 进行数据同步的输入源
    * `ngx_path`: `HA` 部署的目录，`sync server` 会在该目录保存同步状态文件
    * `auth_path`: `HA` 访问 `hustdb` 的用户名密码文件，`sync server` 需要以此作为 `http` 请求的参数

**以下字段根据实际生产环境配置合适的值：**

* `sync`
    * `threads`: 数据同步的线程数
    * `release_interval`: 检测日志文件是否同步完成的时间间隔，如果同步完成，会删除该日志文件。
    * `checkdb_interval`: 检测后台 `db` 存活的时间间隔，只有后台 `db` 存活，才会开始同步
    * `checklog_interval`: 检测日志文件是否已被写完的时间间隔，与 `zlog` 生成单一日志文件的速度保持一致

**除此之外的其他字段均建议保持默认值**。

### 日志配置 ###
  
源码路径：`hustdb/sync/module/zlog.conf`  
安装路径：`data/hustdbsync/zlog.conf`

以下是一个完整的配置文件：

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

如果 `sync server` 的部署路径不是 `/data/hustdbsync`，则其中 `[rules]` 需要变更为对应的路径。


[上一级](../ha.md)

[根目录](../../index.md)