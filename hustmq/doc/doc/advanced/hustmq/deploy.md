部署
--

### 程序 ###

* `hustdb/db/server/make/linux/hustdb`
* `hustdb/db/server/make/linux/hustdb.conf`

### 启动 ###

* `nohup ./hustdb > /dev/null 2>&1 &`

### 配置 ###

    [server]
    tcp.port                        = 8085
    tcp.backlog                     = 512
    tcp.enable_reuseport            = true
    tcp.enable_nodelay              = true
    tcp.enable_defer_accept         = true
    tcp.max_body_size               = 16777216
    tcp.max_keepalive_requests      = 1024      //单链接最大请求数 
    tcp.recv_timeout                = 300       //单链接最长存活时间
    tcp.send_timeout                = 300       //单链接发送数据超时

    tcp.worker_count                = 10        //worker线程数

    http.security.user              = hello     //权限验证，user
    http.security.passwd            = world     //权限验证，password

    http.access.allow               = 127.0.0.1 //IP限制；例如：(1)X.Y.Z.1-X.Y.Z.10；(2)X.Y.Z.1-X.Y.Z.10,X.Y.Z.22；(3)X.Y.Z.1-X.Y.Z.10,X.Y.Z.17,A.B.C.1-A.B.C.10

	# UNIT Second
	redelivery.timeout              = 600       //MQ，message默认处理超时时间

	# UNIT Second
    ttl.def_msg                     = 900       //MQ，message默认ttl
    ttl.max_msg                     = 7200      //MQ，message最大ttl
    ttl.max_kv                      = 2592000   //DB，item最大ttl

	# UNIT Percentage
    memory.process.threshold        = 70        //hustdb内存限制（%），超出阈值，禁止除del外所有写操作
    memory.system.threshold         = 90        //系统内存限制（%），超出阈值，禁止除del外所有写操作

    [fastdb]
    # 1 ~ 20, default 10
    count                           = 10        //hustdb重要模块（没有之一），db实例数
    # UNIT MB, default 256
    l1_cache                        = 512       //fastdb一级缓存
    # UNIT MB, default 512
    l2_cache                        = 8192      //fastdb二级缓存
    # UNIT MB, default 512
    write_buffer                    = 1024      //fastdb写操作缓存
    # default 0
    bloom_filter_bits               = 0
    # default none
    md5_bloom_filter                = none
    # default false
    disable_compression             = false

    [conflictdb]
    # 1 ~ 10, default 2
    count                           = 2         //fastdb重要模块，db实例数
    # UNIT MB, default 128
    cache                           = 128       //conflictdb缓存
    # UNIT MB, default 128
    write_buffer                    = 128       //conflictdb写操作缓存
    # default 0
    bloom_filter_bits               = 0
    # default large
    md5_bloom_filter                = large
    # default true
    disable_compression             = true

    [fast_conflictdb]
    # 1 ~ 10, default 4
    count                           = 4         //fastdb重要模块，单块缓存大小

    [contentdb]
    # enable if count large than 0
    count                           = 256       //fastdb重要模块，db实例数，为0表示禁用；建议：仅用于MQ集群搭建
    # UNIT MB, 16 ~ 128, default 64
    cache                           = 64        //contentdb缓存

[上一级](../index.md)

[根目录](../../index.md)