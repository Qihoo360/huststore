hustdb
--

**安装路径：`/opt/huststore`**

### 程序 ###

* `/opt/huststore/hustdb/hustdb`
* `/opt/huststore/hustdb/hustdb.conf`

### 使用方法 ###

#### 查看当前版本 ####

    $ export LD_LIBRARY_PATH=/opt/huststore/3rd/lib
    $ /opt/huststore/hustdb/hustdb -v

#### 启动服务（守护进程方式，后台运行） ####

    $ export LD_LIBRARY_PATH=/opt/huststore/3rd/lib
    $ /opt/huststore/hustdb/hustdb

#### 退出服务 ####

    $ export LD_LIBRARY_PATH=/opt/huststore/3rd/lib
    $ /opt/huststore/hustdb/hustdb -q

#### 启动服务（调试方式，前台运行） ####

    $ export LD_LIBRARY_PATH=/opt/huststore/3rd/lib
    $ /opt/huststore/hustdb/hustdb -d

### 配置 ###

    [server]
    tcp.port                        = 8085
    tcp.backlog                     = 512
    tcp.enable_reuseport            = true
    tcp.enable_nodelay              = true
    tcp.enable_defer_accept         = true
    tcp.max_body_size               = 33554432
    tcp.max_keepalive_requests      = 8192          //单链接最大请求数 
    tcp.recv_timeout                = 300           //单链接最长存活时间
    tcp.send_timeout                = 300           //单链接发送数据超时

    tcp.worker_count                = 24            //worker线程数

    http.security.user              = huststore     //权限验证，user
    http.security.passwd            = huststore     //权限验证，password

    http.access.allow               = W.X.Y.Z       //IP限制；例如：(1)X.Y.Z.1-X.Y.Z.10；(2)X.Y.Z.1-X.Y.Z.10,X.Y.Z.22；(3)X.Y.Z.1-X.Y.Z.10,X.Y.Z.17,A.B.C.1-A.B.C.10

    # UNIT Percentage(%)
    memory.process.threshold        = 0             //hustdb进程内存限制（%），超出阈值，禁止除del外所有写操作
    memory.system.threshold         = 0             //系统内存限制（%），超出阈值，禁止除del外所有写操作

    [store]
    # UNIT Minute, 1 ~ 255
    mq.redelivery.timeout           = 5             //MQ，message默认处理超时时间

    # UNIT Second
    mq.ttl.maximum                  = 7200          //MQ，message最大存活时间
    db.ttl.maximum                  = 2592000       //DB，item最大存活时间
    db.ttl.scan_interval            = 30            //DB，ttl扫描间隔，定期回收ttl超时的item

    # 1 ~ 10000, default 1000
    db.ttl.scan_count               = 1000          //DB，每次ttl扫描的item数量

    db.binlog.thread_count          = 4             //DB，binlog的worker线程数
    db.binlog.queue_capacity        = 4000          //DB，binlog任务队列容量

    # UNIT Second
    db.binlog.scan_interval         = 20            //DB，binlog扫描间隔，定期同步binlog的item
    db.binlog.task_timeout          = 950400        //DB，binlog任务超时时间

    # UNIT Percentage(%), default 100
    db.post_compression.ratio       = 100           //DB, (post compression ratio) = (compressed data size / raw data size), 默认设置100，即禁用压缩

    mq.queue.maximum                = 8192          //MQ，queue最大数量；当且仅当，数值修改：大→小，会造成尾部索引失效，改回原数值即可恢复
    db.table.maximum                = 8192          //DB，table最大数量；当且仅当，数值修改：大→小，会造成尾部索引失效，改回原数值即可恢复

    [cachedb]
    # UNIT MB, default 2048
    cache                           = 2048          //CACHE，独立于DB，仅用于缓存

    [md5db]
    # 1 ~ 20, default 10
    count                           = 10            //hustdb重要模块，db实例数（首次初始化后，禁止修改）
    # UNIT MB, default 256
    l1_cache                        = 256           //md5db一级缓存
    # UNIT MB, default 1024
    l2_cache                        = 1024          //md5db二级缓存
    # UNIT MB, default 1024
    write_buffer                    = 1024          //md5db写操作缓存
    # default 0
    bloom_filter_bits               = 0
    # default true
    disable_compression             = true

    [contentdb]
    # enable if count large than 0, default 256
    count                           = 256           //CONTENTDB，建议启用，尤其针对大value数据，db实例数（首次初始化后，禁止修改）

    [conflictdb]
    # 1 ~ 10, default 2
    count                           = 2             //md5db重要模块，db实例数（首次初始化后，禁止修改）
    # UNIT MB, default 128
    cache                           = 128           //conflictdb缓存
    # UNIT MB, default 128
    write_buffer                    = 128           //conflictdb写操作缓存
    # default 10
    bloom_filter_bits               = 10
    # default true
    disable_compression             = true

    [fast_conflictdb]
    # 1 ~ 10, default 4
    count                           = 4             //md5db重要模块，单块缓存大小（首次初始化后，禁止修改）

[上一页](index.md)

[回首页](../index.md)