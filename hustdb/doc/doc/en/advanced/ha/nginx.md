Nginx Configuration
--

Configuration file Path: `hustdb/ha/nginx/conf/nginx.json`

### Configuration Examples ###

A complete example of configuration file: 

    {
        "module": "hustdb_ha",
        "worker_processes": 4,
        "worker_connections": 1048576,
        "listen": 8082,
        "keepalive_timeout": 540,
        "keepalive": 32768,
        "http_basic_auth_file": "/opt/huststore/hustdbha/conf/htpasswd",
        "nginx_root": "/opt/huststore/hustdbha/html",
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


**Set a suitable value for the below fields in your specific network environment**

* `listen`: Listening port
* `http_basic_auth_file`: configuration file for `http basic authentication`
use **plain text** to save authentication to ensure the high performance of `hustdb ha`
* `proxy`
    * `auth`: the authentication string (encoded in `base64`) for `hustdb`  `http basic authentication`
    * `backends`: machine list configuration for `hustdb`

Below fields in `main_conf` are used in [`ngx_http_fetch`](../../../../../hustmq/doc/doc/advanced/ha/components.md):

* `fetch_req_pool_size`: Memory pool for each sub request, default value recommended
* `keepalive_cache_size`: Number of `keepalive` connections set up by `ngx_http_fetch` and `hustmq`
* `connection_cache_size`: Size of connection pool of `ngx_http_fetch`, default value recommended
* `fetch_connect_timeout`: Connection timeout of `ngx_http_fetch`, set a suitable value to your specific network environment
* `fetch_send_timeout`: Package send timeout of `ngx_http_fetch`, set a suitable value to your specific network environment
* `fetch_read_timeout`: Package read timeout of `ngx_http_fetch`, set a suitable value to your specific network environment
* `fetch_buffer_size`: Buffer size for sending/receiving packages of `ngx_http_fetch`, default value recommended

Below fields in `main_conf` are used in communication with `sync_server`: 

* `sync_port`: Listening port of `sync_server`, **keep it the same as** in [`sync_server` configuration](sync_conf.md) 
* `sync_status_uri`: Status request service address of `sync_server`, **Use default value!**
* `sync_user`: Username for `sync_server` to do `http basic authentication`, **keep it the same as** in [`sync_server` configuration](sync_conf.md)
* `sync_passwd`: Password for`sync_server` to do `http basic authentication`, **keep it the same as** in [`sync_server` configuration](sync_conf.md)

**Below fields are related to [set_table](../../api/ha/set_table.md)**
  
* `hustdb_ha_shm_name`: Name of shared memory  
* `hustdb_ha_shm_size`: Size of shared memory  
* `public_pem`: Public key of RSA  
* `identifier_cache_size`: Number of cached id
* `identifier_timeout`: Timout of each assigned id

Default value are recommended for the above fields. In particular for `identifier_timeout`, it is not recommended to set a big value **considering the security issues**.

**Except of this, other fields are recommended to use the default values.**

The meaning and configuration method of most field are the same as nginx configuration, e.g.

* `worker_processes`
* `keepalive_timeout`
* `keepalive`
* `proxy_connect_timeout`
* `proxy_send_timeout`
* `proxy_read_timeout`
* `proxy_buffer_size`

Configuration of `health_check`, please check [`nginx_upstream_check_module`](https://github.com/yaoweibin/nginx_upstream_check_module)

### Tool for generate configuration file ###

Path: `hustdb/ha/nginx/conf/genconf.py`

After complete configurate `nginx.json`, use `genconf.py` to generate `nginx.conf` for production environment.

Command: 

    python genconf.py nginx.json

Content of the generated `nginx.conf`:

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
            fetch_buffer_size         64m;
            sync_port                 8089;
            sync_status_uri           /sync_status;
            sync_user                 sync;
            sync_passwd               sync;
            binlog_uri                /hustdb/binlog;

            location /status.html {
                root /opt/huststore/hustdbha/html;
            }

            location /put {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /get {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /get2 {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /del {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /exist {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /keys {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /hset {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /hget {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /hget2 {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /hdel {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /hexist {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /hkeys {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /sadd {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /srem {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /sismember {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /sismember2 {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /smembers {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /zadd {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /zrem {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /zismember {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /zscore {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /zscore2 {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /zrangebyrank {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /zrangebyscore {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /stat {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /stat_all {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /file_count {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /peer_count {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /sync_status {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /sync_alive {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /get_table {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /set_table {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/exist {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/get {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/ttl {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/put {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/append {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/del {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/expire {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/persist {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/hexist {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/hget {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/hset {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/hdel {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/hincrby {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
            }
            location /cache/hincrbyfloat {
                hustdb_ha;
                http_basic_auth_file /opt/huststore/hustdbha/conf/htpasswd;
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

### FAQ ###

* How to disable `http basic authentication`?  
In `nginx.json`, delete `"http_basic_auth_file"` field and its value, use `genconf.py` script to re-generate `nginx.conf`.

[Previous](conf.md)

[Home](../../index.md)