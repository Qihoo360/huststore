Deployment
--

### Program ###

* `hustdb/db/server/make/linux/hustdb`
* `hustdb/db/server/make/linux/hustdb.conf`

### Usage ###

#### Service Start (Daemon, work in background) ####

    ./hustdb

#### Service Stop ####

    ./hustdb -q

#### Service Start (Debug, work in foreground) ####

    ./hustdb -d

### Configuration ###

    [server]
    tcp.port                        = 8085
    tcp.backlog                     = 512
    tcp.enable_reuseport            = true
    tcp.enable_nodelay              = true
    tcp.enable_defer_accept         = true
    tcp.max_body_size               = 16777216
    tcp.max_keepalive_requests      = 1024          //Max number of requests for a single connection单链接最大请求数 
    tcp.recv_timeout                = 300           //Max live time for for a single connection单链接最长存活时间
    tcp.send_timeout                = 300           //Timeout for a single connection单链接发送数据超时

    tcp.worker_count                = 10            //Number of worker thread

    http.security.user              = huststore     //Authority verification: user
    http.security.passwd            = huststore     //Authority verrification: password

    http.access.allow               = 127.0.0.1     //IP restrictions. e.g. (1) X.Y.Z.1-X.Y.Z.10;(2) X.Y.Z.1-X.Y.Z.10,X.Y.Z.22;(3) X.Y.Z.1-X.Y.Z.10,X.Y.Z.17,A.B.C.1-A.B.C.10

	# UNIT Percentage
    memory.process.threshold        = 70            //Process memory limit (%) for hustdb , if exceeded, all write operations, except del, will be disabled.
    memory.system.threshold         = 90            //System memory limit, if exceed, all write operations, except del, will be disabled.

	[store]
	# UNIT Minute, 1 ~ 255
	mq.redelivery.timeout           = 5             //MQ, the default message process time 

	# UNIT Second
	mq.ttl.maximum                  = 7200          //MQ, max number of time to live for message
	db.ttl.maximum                  = 2592000       //DB, max number of time to live for item
    db.ttl.scan_interval            = 30            //DB, scan interval for ttl, timeout item will be recycled periodically

	mq.queue.maximum                = 8192          //MQ, max capacity of queue; If it's value is reduced, the tail index will become invalid, change it to the original value to restore.
	db.table.maximum                = 8192          //DB, max capacity of table; If it's value is reduced, the tail index will become invalid, change it to the original value to restore.

    # 1 ~ 10000, default 1000
    db.ttl.scan_count               = 1000          //DB, number of item each time ttl scan is executed.

    [cachedb]
    # UNIT MB, default 512
    cache                           = 8192          //CACHE, independent of DB, used only for cache.

    [fastdb]
    # 1 ~ 20, default 10
    count                           = 10            //The most important module of hustdb, number of instance of db (Modification is forbidden after initialization).
    # UNIT MB, default 256
    l1_cache                        = 512           //L1 cache of fastdb
    # UNIT MB, default 512
    l2_cache                        = 8192          //L2 cache of fastdb
    # UNIT MB, default 512
    write_buffer                    = 1024          //Write cache of fastdb
    # default 0
    bloom_filter_bits               = 0
    # default none
    md5_bloom_filter                = none
    # default false
    disable_compression             = false

    [conflictdb]
    # 1 ~ 10, default 2
    count                           = 2             //An important module of fastdb, number of instance of db (Modification is forbidden after initialization).
    # UNIT MB, default 128
    cache                           = 128           //Cache of conflictdb
    # UNIT MB, default 128
    write_buffer                    = 128           //Write cache of conflictdb
    # default 0
    bloom_filter_bits               = 0
    # default large
    md5_bloom_filter                = large
    # default true
    disable_compression             = true

    [fast_conflictdb]
    # 1 ~ 10, default 4
    count                           = 4             //An important module of fastdb, size of single chunk of cache (Modification is forbidden after initialization).
    [contentdb]
    # enable if count large than 0
    count                           = 256             //An important module of fastdb, number of instance of db. 0 means not allowed. Recommendation: use it only for building MQ cluster (Modification is forbidden after initialization).
    # UNIT MB, 16 ~ 128, default 64
    cache                           = 64            //Cache of contentdb

[Previous](../index.md)

[Home](../../index.md)