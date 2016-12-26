hustdb
--

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

    tcp.worker_count                = 16            //worker线程数

    http.security.user              = huststore     //权限验证，user
    http.security.passwd            = huststore     //权限验证，password

    http.access.allow               = W.X.Y.Z       //IP限制；例如：(1)X.Y.Z.1-X.Y.Z.10；(2)X.Y.Z.1-X.Y.Z.10,X.Y.Z.22；(3)X.Y.Z.1-X.Y.Z.10,X.Y.Z.17,A.B.C.1-A.B.C.10

    # UNIT Percentage
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

    mq.queue.maximum                = 8192          //MQ，queue最大数量；当且仅当，数值修改：大→小，会造成尾部索引失效，改回原数值即可恢复
    db.table.maximum                = 8192          //DB，table最大数量；当且仅当，数值修改：大→小，会造成尾部索引失效，改回原数值即可恢复

    [cachedb]
    # UNIT MB, default 512
    cache                           = 8192          //CACHE，独立于DB，仅用于缓存

    [fastdb]
    # 1 ~ 20, default 10
    count                           = 10            //hustdb重要模块（没有之一），db实例数（首次初始化后，禁止修改）
    # UNIT MB, default 256
    l1_cache                        = 512           //fastdb一级缓存
    # UNIT MB, default 512
    l2_cache                        = 8192          //fastdb二级缓存
    # UNIT MB, default 512
    write_buffer                    = 1024          //fastdb写操作缓存
    # default 0
    bloom_filter_bits               = 0
    # default none
    md5_bloom_filter                = none
    # default false
    disable_compression             = false

    [conflictdb]
    # 1 ~ 10, default 2
    count                           = 2             //fastdb重要模块，db实例数（首次初始化后，禁止修改）
    # UNIT MB, default 128
    cache                           = 128           //conflictdb缓存
    # UNIT MB, default 128
    write_buffer                    = 128           //conflictdb写操作缓存
    # default 0
    bloom_filter_bits               = 0
    # default large
    md5_bloom_filter                = large
    # default true
    disable_compression             = true

    [fast_conflictdb]
    # 1 ~ 10, default 4
    count                           = 4             //fastdb重要模块，单块缓存大小（首次初始化后，禁止修改）

    [contentdb]
    # enable if count large than 0
    count                           = 0             //fastdb重要模块，db实例数，为0表示禁用；建议：仅用于MQ集群搭建（首次初始化后，禁止修改）
    # UNIT MB, 16 ~ 128, default 64
    cache                           = 64            //contentdb缓存

[上一页](index.md)

[回首页](../index.md)