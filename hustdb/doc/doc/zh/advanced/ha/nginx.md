nginx 配置文件
--

路径：`hustdb/ha/nginx/conf/nginx.json`

### 配置范例 ###

以下是一个完整的配置文件：

    {
        "module": "hustdb_ha",
        "worker_processes": 4,
        "worker_connections": 1048576,
        "listen": 8082,
        "keepalive_timeout": 540,
        "keepalive": 32768,
        "http_basic_auth_file": "/data/hustdbha/conf/htpasswd",
        "nginx_root": "/data/hustdbha/html",
        "main_conf":
        [
            ["zlog_mdc", "sync_dir"],
            ["hustdbtable_file", "hustdbtable.json"],
            ["hustdb_ha_shm_name", "hustdb_ha_share_memory"],
            ["hustdb_ha_shm_size", "10m"],
            ["public_pem", "public.pem"],
            ["identifier_cache_size", 128],
            ["identifier_timeout", "10s"],
            ["fetch_req_pool_size", "4k"],
            ["keepalive_cache_size", 1024],
            ["connection_cache_size", 1024],
            ["fetch_connect_timeout", "2s"],
            ["fetch_send_timeout", "60s"],
            ["fetch_read_timeout", "60s"],
            ["fetch_timeout", "60s"],
            ["fetch_buffer_size", "64m"],
            ["sync_port", "8089"],
            ["sync_status_uri", "/sync_status"],
            ["sync_user", "sync"],
            ["sync_passwd", "sync"],
            ["binlog_uri", "/hustdb/binlog"]
        ],
        "auth_filter": [],
        "local_cmds": 
        [
            "put",
            "get",
            "get2",
            "del",
            "exist",
            "keys",
            "hset",
            "hget",
            "hget2",
            "hdel",
            "hexist",
            "hkeys",
            "sadd",
            "srem",
            "sismember",
            "sismember2",
            "smembers",
            "zadd",
            "zrem",
            "zismember",
            "zscore",
            "zscore2",
            "zrangebyrank",
            "zrangebyscore",
            "stat",
            "stat_all",
            "file_count",
            "peer_count",
            "sync_status",
            "sync_alive",
            "get_table",
            "set_table",
            "cache/exist",
            "cache/get",
            "cache/ttl",
            "cache/put",
            "cache/append",
            "cache/del",
            "cache/expire",
            "cache/persist",
            "cache/hexist",
            "cache/hget",
            "cache/hset",
            "cache/hdel", 
            "cache/hincrby",
            "cache/hincrbyfloat"
        ],
        "proxy":
        {
            "health_check": 
            [
                "check interval=5000 rise=1 fall=3 timeout=5000 type=http",
                "check_http_send \"GET /status.html HTTP/1.1\\r\\n\\r\\n\"",
                "check_http_expect_alive http_2xx"
            ],
            "auth": "aHVzdHN0b3JlOmh1c3RzdG9yZQ==",
            "proxy_connect_timeout": "2s",
            "proxy_send_timeout": "60s",
            "proxy_read_timeout": "60s",
            "proxy_buffer_size": "64m",
            "backends": 
            [
                "192.168.1.101:9999", 
                "192.168.1.102:9999", 
                "192.168.1.103:9999", 
                "192.168.1.104:9999", 
                "192.168.1.105:9999", 
                "192.168.1.106:9999"
            ],
            "proxy_cmds":
            [
                "/hustdb/put",
                "/hustdb/get", 
                "/hustdb/del", 
                "/hustdb/exist",
                "/hustdb/keys", 
                "/hustdb/hset", 
                "/hustdb/hget", 
                "/hustdb/hdel", 
                "/hustdb/hexist", 
                "/hustdb/hkeys",
                "/hustdb/sadd", 
                "/hustdb/srem", 
                "/hustdb/sismember", 
                "/hustdb/smembers",
                "/hustdb/zadd",
                "/hustdb/zrem",
                "/hustdb/zismember",
                "/hustdb/zscore",
                "/hustdb/zrangebyrank",
                "/hustdb/zrangebyscore",
                "/hustdb/stat",
                "/hustdb/stat_all",
                "/hustdb/file_count",
                "/hustdb/binlog",
                "/hustcache/exist",
                "/hustcache/get",
                "/hustcache/ttl",
                "/hustcache/put",
                "/hustcache/append",
                "/hustcache/del",
                "/hustcache/expire",
                "/hustcache/persist",
                "/hustcache/hexist",
                "/hustcache/hget",
                "/hustcache/hset",
                "/hustcache/hdel", 
                "/hustcache/hincrby",
                "/hustcache/hincrbyfloat"
            ]
        }
    }


**以下字段根据实际生产环境配置合适的值：**

* `listen`: 监听端口
* `http_basic_auth_file`: `http basic authentication` 的配置文件  
（为保证 `hustdb ha` 的高性能，文件使用 **明文** 来配置认证信息）
* `proxy`
    * `auth`: `hustdb` 的 `http basic authentication` 认证字符串（进行 `base64` 加密）
    * `backends`: `hustdb` 机器列表配置

`main_conf` 中的如下字段均用于 [`ngx_http_fetch`](../../../../../hustmq/doc/doc/advanced/ha/components.md) :

* `fetch_req_pool_size`：`ngx_http_fetch` 每个子请求申请的内存池大小，建议保持默认值
* `keepalive_cache_size`：`ngx_http_fetch` 和 `hustmq` 机器所建立的连接中，保持 `keepalive` 状态的连接数量，建议保持默认值
* `connection_cache_size`：`ngx_http_fetch` 连接池的大小，建议保持默认值
* `fetch_connect_timeout`：`ngx_http_fetch` 连接的超时时间，可根据网络环境配置合适的值
* `fetch_send_timeout`：`ngx_http_fetch` 发送数据包的超时时间，可根据网络环境配置合适的值
* `fetch_read_timeout`：`ngx_http_fetch` 接收数据包的超时时间，可根据网络环境配置合适的值
* `fetch_timeout`：`ngx_http_fetch` 和 `hustmq` 进行网络通讯的最大超时时间，可根据网络环境配置合适的值
* `fetch_buffer_size`：`ngx_http_fetch` 收发数据包的缓冲区大小，建议保持默认值

`main_conf` 中的如下字段均用于和 `sync_server` 的通信：

* `sync_port`: `sync_server` 监听的端口，请和 [`sync_server` 的配置](sync_conf.md) **保持一致**
* `sync_status_uri`: `sync_server` 提供的状态请求服务地址，**请保持默认值**
* `sync_user`: `sync_server` 进行 `http basic authentication` 的用户名，请和 [`sync_server` 的配置](sync_conf.md) **保持一致**
* `sync_passwd`: `sync_server` 进行 `http basic authentication` 的密码，请和 [`sync_server` 的配置](sync_conf.md) **保持一致**

**以下配置和 [set_table](../../api/ha/set_table.md) 相关**
  
* `hustdb_ha_shm_name`: 共享内存的名字  
* `hustdb_ha_shm_size`: 共享内存的大小  
* `public_pem`: RSA 加密的公钥  
* `identifier_cache_size`: id 缓存的数量  
* `identifier_timeout`: 分配的 id 的有效时长

以上字段建议保持默认值。尤其是 `identifier_timeout` ， **出于安全性的考虑** ，不建议设置过大的值。

**除此之外的其他字段均建议保持默认值**。

大部分字段的含义以及配置方法和 nginx 官方的配置文件的含义一致，例如：

* `worker_processes`
* `keepalive_timeout`
* `keepalive`
* `proxy_connect_timeout`
* `proxy_send_timeout`
* `proxy_read_timeout`
* `proxy_buffer_size`

`health_check` 的配置可参考 [`nginx_upstream_check_module`](https://github.com/yaoweibin/nginx_upstream_check_module)

### 配置文件生成工具 ###

路径：`hustdb/ha/nginx/conf/genconf.py`

配置好 `nginx.json` 之后，可使用 `genconf.py` 生成实际生产环境使用的配置文件 `nginx.conf`，命令如下：

    python genconf.py nginx.json

生成之后的 `nginx.conf` 内容如下：

    worker_processes  4;
    daemon on;
    master_process on;

    events {
        use epoll;
        multi_accept on;
        worker_connections  1048576;
    }

    http {
        include                      mime.types;
        default_type                 application/octet-stream;

        sendfile                     on;
        keepalive_timeout            540;

        client_body_timeout          10;
        client_header_timeout        10;

        client_header_buffer_size    1k;
        large_client_header_buffers  4  4k;
        output_buffers               1  32k;
        client_max_body_size         64m;
        client_body_buffer_size      2m;

        upstream backend {
            customized_selector;
            server 192.168.1.101:9999;
            server 192.168.1.102:9999;
            server 192.168.1.103:9999;
            server 192.168.1.104:9999;
            server 192.168.1.105:9999;
            server 192.168.1.106:9999;
            check interval=5000 rise=1 fall=3 timeout=5000 type=http;
            check_http_send "GET /status.html HTTP/1.1\r\n\r\n";
            check_http_expect_alive http_2xx;
            keepalive 32768;
        }

        server {
            listen                    8082;
            #server_name              hostname;
            access_log                /dev/null;
            error_log                 /dev/null;
            chunked_transfer_encoding off;
            keepalive_requests        32768;
            zlog_mdc                  sync_dir;
            hustdbtable_file          hustdbtable.json;
            hustdb_ha_shm_name        hustdb_ha_share_memory;
            hustdb_ha_shm_size        10m;
            public_pem                public.pem;
            identifier_cache_size     128;
            identifier_timeout        10s;
            fetch_req_pool_size       4k;
            keepalive_cache_size      1024;
            connection_cache_size     1024;
            fetch_connect_timeout     2s;
            fetch_send_timeout        60s;
            fetch_read_timeout        60s;
            fetch_timeout             60s;
            fetch_buffer_size         64m;
            sync_port                 8089;
            sync_status_uri           /sync_status;
            sync_user                 sync;
            sync_passwd               sync;
            binlog_uri                /hustdb/binlog;

            location /status.html {
                root /data/hustdbha/html;
            }

            location /put {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /get {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /get2 {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /del {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /exist {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /keys {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /hset {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /hget {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /hget2 {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /hdel {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /hexist {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /hkeys {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /sadd {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /srem {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /sismember {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /sismember2 {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /smembers {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /zadd {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /zrem {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /zismember {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /zscore {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /zscore2 {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /zrangebyrank {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /zrangebyscore {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /stat {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /stat_all {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /file_count {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /peer_count {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /sync_status {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /sync_alive {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /get_table {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /set_table {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/exist {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/get {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/ttl {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/put {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/append {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/del {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/expire {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/persist {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/hexist {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/hget {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/hset {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/hdel {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/hincrby {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /cache/hincrbyfloat {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }

            location /hustdb/put {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/get {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/del {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/exist {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/keys {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/hset {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/hget {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/hdel {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/hexist {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/hkeys {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/sadd {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/srem {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/sismember {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/smembers {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/zadd {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/zrem {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/zismember {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/zscore {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/zrangebyrank {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/zrangebyscore {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/stat {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/stat_all {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/file_count {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustdb/binlog {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/exist {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/get {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/ttl {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/put {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/append {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/del {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/expire {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/persist {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/hexist {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/hget {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/hset {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/hdel {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/hincrby {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
            location /hustcache/hincrbyfloat {
                proxy_pass http://backend;
                proxy_http_version 1.1;
                proxy_set_header Connection "Keep-Alive";
                proxy_set_header Authorization "Basic aHVzdHN0b3JlOmh1c3RzdG9yZQ==";
                proxy_connect_timeout 2s;
                proxy_send_timeout 60s;
                proxy_read_timeout 60s;
                proxy_buffer_size 64m;
                proxy_buffers 2 64m;
                proxy_busy_buffers_size 64m;
            }
        }
    }

### 常见问题 ###

* 如何禁用 `http basic authentication` ？  
配置 `nginx.json` 时，直接删掉 `"http_basic_auth_file"` 字段以及相应的值，利用 `genconf.py` 重新生成 `nginx.conf` 即可

[上一页](conf.md)

[回首页](../../index.md)