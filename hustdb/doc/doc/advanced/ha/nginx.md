nginx 配置文件
--

路径：`hustdb/ha/nginx/conf/nginx.json`

### 配置范例 ###

以下是一个完整的配置文件：

    {
        "module": "hustdb_ha",
        "worker_connections": 1048576,
        "listen": 8082,
        "keepalive_timeout": 540,
        "keepalive": 32768,
        "http_basic_auth_file": "/data/hustdbha/conf/htpasswd",
        "nginx_root": "/data/hustdbha/html",
        "main_conf":
        [
            ["sync_threads", 4],
            ["sync_release_interval", "5s"],
            ["sync_checkdb_interval", "5s"],
            ["sync_checklog_interval", "60s"],
            ["zlog_mdc", "sync_dir"],
            ["hustdbtable_file", "hustdbtable.json"],
            ["hustdb_ha_shm_name", "hustdb_ha_share_memory"],
            ["hustdb_ha_shm_size", "10m"],
            ["public_pem", "public.pem"],
            ["identifier_cache_size", 128],
            ["identifier_timeout", "10s"]
        ],
        "auth_filter": [],
        "local_cmds": 
        [
            "put",
            "get",
            "del",
            "exist",
            "keys",
            "hset",
            "hget",
            "hdel",
            "hexist",
            "hkeys",
            "sadd",
            "srem",
            "sismember",
            "smembers",
            "stat",
            "stat_all",
            "export",
            "file_count",
            "peer_count",
            "sync_status",
            "get_table",
            "set_table"
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
                "/hustdb/stat",
                "/hustdb/stat_all",
                "/hustdb/export", 
                "/hustdb/file_count"
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

**以下配置和 [set_table](../../api/ha/set_table.md) 相关**
  
* `hustdb_ha_shm_name`: 共享内存的名字  
* `hustdb_ha_shm_size`: 共享内存的大小  
* `public_pem`: RSA 加密的公钥  
* `identifier_cache_size`: id 缓存的数量  
* `identifier_timeout`: 分配的 id 的有效时长

以上字段建议保持默认值。尤其是 `identifier_timeout` ， **出于安全性的考虑** ，不建议设置过大的值。

**除此之外的其他字段均建议保持默认值**。

大部分字段的含义以及配置方法和 nginx 官方的配置文件的含义一致，例如：

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

    worker_processes  1;
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
            sync_threads              4;
            sync_release_interval     5s;
            sync_checkdb_interval     5s;
            sync_checklog_interval    60s;
            zlog_mdc                  sync_dir;
            hustdbtable_file          hustdbtable.json;
            hustdb_ha_shm_name        hustdb_ha_share_memory;
            hustdb_ha_shm_size        10m;
            public_pem                public.pem;
            identifier_cache_size     128;
            identifier_timeout        10s;

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
            location /smembers {
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
            location /export {
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
            location /get_table {
                hustdb_ha;
                http_basic_auth_file /data/hustdbha/conf/htpasswd;
            }
            location /set_table {
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
            location /hustdb/export {
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
        }
    }

### 常见问题 ###

* 如何禁用 `http basic authentication` ？  
配置 `nginx.json` 时，直接删掉 `"http_basic_auth_file"` 字段以及相应的值，利用 `genconf.py` 重新生成 `nginx.conf` 即可

[上一级](conf.md)

[根目录](../../index.md)