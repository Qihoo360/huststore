nginx 配置文件
--

路径：`hustmq/ha/nginx/conf/nginx.json`

### 配置范例 ###

以下是一个完整的配置文件：

    {
        "module": "hustmq_ha",
        "worker_connections": 1048576,
        "listen": 8080,
        "keepalive_timeout": 540,
        "keepalive": 32768,
        "http_basic_auth_file": "/data/hustmqha/conf/htpasswd",
        "nginx_root": "/data/hustmqha/html",
        "auth_filter": ["get", "sub"],
        "local_cmds":
        [
            "autost", "stat_all", "stat",
            "put", "get", "ack", "timeout", 
            "lock", "max", "purge", 
            "worker", "evget", "evsub", 
            "sub", "pub", "do_get", "do_post",
            "do_get_status", "do_post_status"
        ],
        "main_conf":
        [
            ["max_queue_size", 8192],
            ["queue_hash", "on"],
            ["long_polling_timeout", "180s"],
            ["subscribe_timeout", "180s"],
            ["publish_timeout", "180s"],
            ["max_timer_count", "1m"],
            ["status_cache", "off"],
            ["fetch_req_pool_size", "4k"],
            ["keepalive_cache_size", 1024],
            ["connection_cache_size", 1024],
            ["autost_uri", "/hustmq/stat_all"],
            ["username", "huststore"],
            ["password", "huststore"],
            ["fetch_connect_timeout", "2s"],
            ["fetch_send_timeout", "60s"],
            ["fetch_read_timeout", "60s"],
            ["fetch_timeout", "60s"],
            ["fetch_buffer_size", "64m"],
            ["autost_interval", "200ms"],
            ["do_post_cache_size", 1024],
            ["do_get_cache_size", 1024],
            ["max_do_task_body_size", "8m"],
            ["do_post_timeout", "180s"],
            ["do_get_timeout", "180s"]
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
            "backends": ["192.168.1.101:9999"],
            "proxy_cmds":
            [
                "/hustmq/stat_all",
                "/hustmq/put",
                "/hustmq/get",
                "/hustmq/ack",
                "/hustmq/timeout",
                "/hustmq/lock",
                "/hustmq/max",
                "/hustmq/purge",
                "/hustmq/worker",
                "/hustmq/sub",
                "/hustmq/pub"
            ]
        }
    }

**以下字段根据实际生产环境配置合适的值：**

* `listen`: 监听端口
* `http_basic_auth_file`: `http basic authentication` 的配置文件  
（为保证 `hustmq ha` 的高性能，文件使用 **明文** 来配置认证信息）
* `proxy`
    * `auth`: `hustmq` 的 `http basic authentication` 认证字符串（进行 `base64` 加密）
    * `backends`: `hustmq` 机器列表配置

其中 `main_conf` 中的如下字段均用于 [`ngx_http_fetch`](components.md) :

* `fetch_req_pool_size`：`ngx_http_fetch` 每个子请求申请的内存池大小，建议保持默认值
* `keepalive_cache_size`：`ngx_http_fetch` 和 `hustmq` 机器所建立的连接中，保持 `keepalive` 状态的连接数量，建议保持默认值
* `connection_cache_size`：`ngx_http_fetch` 连接池的大小，建议保持默认值
* `username`：`hustmq` 机器进行 `http basic authentication` 认证的用户名，**请根据生产环境的实际情况进行配置**
* `password`：`hustmq` 机器进行 `http basic authentication` 认证的密码，**请根据生产环境的实际情况进行配置**
* `fetch_connect_timeout`：`ngx_http_fetch` 连接的超时时间，可根据网络环境配置合适的值
* `fetch_send_timeout`：`ngx_http_fetch` 发送数据包的超时时间，可根据网络环境配置合适的值
* `fetch_read_timeout`：`ngx_http_fetch` 接收数据包的超时时间，可根据网络环境配置合适的值
* `fetch_timeout`：`ngx_http_fetch` 和 `hustmq` 进行网络通讯的最大超时时间，可根据网络环境配置合适的值
* `fetch_buffer_size`：`ngx_http_fetch` 收发数据包的缓冲区大小，建议保持默认值
* `autost_interval`：`hustmq ha` 自动更新集群状态的时间间隔，该字段建议保持默认值

**`queue_hash` 参数详解**  

`main_conf` 中 `queue_hash` 字段用于配置负载均衡的方式，用于 [`put`](../../api/ha/put.md) 接口。

配置为 `on`，表示根据队列名称计算 `hash` 来进行负载均衡。

* 优势：可以保证当存储节点正常工作时，取队列数据的顺序是固定的（先进先出）。  
* 劣势：当针对一个固定队列写入数据时，`QPS` 将受限于单个存储节点。  

配置为 `off`，将以默认的 `round robin` （轮询）实现来进行负载均衡。  

* 优势：当针对一个固定队列写入数据时，`QPS` 不会受限于单个存储节点。  
* 劣势：取队列数据的顺序不是固定的。  


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

路径：`hustmq/ha/nginx/conf/genconf.py`

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
            check interval=5000 rise=1 fall=3 timeout=5000 type=http;
            check_http_send "GET /status.html HTTP/1.1\r\n\r\n";
            check_http_expect_alive http_2xx;
            keepalive 32768;
        }

        server {
            listen                    8080;
            #server_name              hostname;
            access_log                /dev/null;
            error_log                 /dev/null;
            chunked_transfer_encoding off;
            keepalive_requests        32768;
            max_queue_size            8192;
            queue_hash                on;
            long_polling_timeout      180s;
            subscribe_timeout         180s;
            publish_timeout           180s;
            max_timer_count           1m;
            status_cache              off;
            fetch_req_pool_size       4k;
            keepalive_cache_size      1024;
            connection_cache_size     1024;
            autost_uri                /hustmq/stat_all;
            username                  huststore;
            password                  huststore;
            fetch_connect_timeout     2s;
            fetch_send_timeout        60s;
            fetch_read_timeout        60s;
            fetch_timeout             60s;
            fetch_buffer_size         64m;
            autost_interval           200ms;
            do_post_cache_size        1024;
            do_get_cache_size         1024;
            max_do_task_body_size     8m;
            do_post_timeout           180s;
            do_get_timeout            180s;

            location /status.html {
                root /data/hustmqha/html;
            }

            location /autost {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /stat_all {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /stat {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /put {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /get {
                hustmq_ha;
                #http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /ack {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /timeout {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /lock {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /max {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /purge {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /worker {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /evget {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /evsub {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /sub {
                hustmq_ha;
                #http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /pub {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /do_get {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /do_post {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /do_get_status {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }
            location /do_post_status {
                hustmq_ha;
                http_basic_auth_file /data/hustmqha/conf/htpasswd;
            }

            location /hustmq/stat_all {
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
            location /hustmq/put {
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
            location /hustmq/get {
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
            location /hustmq/ack {
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
            location /hustmq/timeout {
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
            location /hustmq/lock {
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
            location /hustmq/max {
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
            location /hustmq/purge {
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
            location /hustmq/worker {
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
            location /hustmq/sub {
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
            location /hustmq/pub {
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

[上一级](index.md)

[根目录](../../index.md)