Nginx Configuration File
--

Path: `hustmq/ha/nginx/conf/nginx.json`

### Configuration Examples ###

The following is complete configuration file:

    {
        "module": "hustmq_ha",
        "worker_connections": 1048576,
        "listen": 8080,
        "keepalive_timeout": 540,
        "keepalive": 32768,
        "http_basic_auth_file": "/opt/huststore/hustmqha/conf/htpasswd",
        "nginx_root": "/opt/huststore/hustmqha/html",
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

**Please configure the appropriate value based on actual production environment for the following fields**

* `listen`: listening port
* `http_basic_auth_file`: the configuration file of `http basic authentication`    
(To ensure the high performance of `hustmq ha`, please use **Plain Text** to configure certification information)
* `main_conf`
    * `username`: `http basic authentication` user name used by `hustmq` machine
    * `password`: `http basic authentication` password used by `hustmq` machine
    * `autost_interval`: Automatically update the cluster status interval used by `hustmq ha` 
* `proxy`
    * `auth`: `http basic authentication` authentication string (`base64` encryption) used by `hustmq` machine
    * `backends`: configuration of `hustmq` machine list

The field of `main_conf` is used [`ngx_http_fetch`](components.md):

* `fetch_req_pool_size`: Memory pool size of each sub-request application for `nginx http fetch`, and the default value is recommended.
* `keepalive_cache_size`: The number of `keepalive` connection of `ngx_http_fetch` and `hustmq`, and the default value is recommended.
* `connection_cache_size`:  Memory pool size of `ngx_http_fetch, and the default value is recommended.
* `fetch_connect_timeout`: Connection timeout of `ngx_http_fetch`, and you can configure the appropriate value based on the actual network environment
* `fetch_send_timeout`: Send data timeout of `ngx_http_fetch`, and you can configure the appropriate value based on the actual network environment
* `fetch_read_timeout`: Receive data timeout of `ngx_http_fetch`, and you can configure the appropriate value based on the actual network environment
* `fetch_timeout`: The maximum network communication timeout between `ngx_http_fetch` and `hustmq`, and you can configure the appropriate value based on the actual network environment.
* `fetch_buffer_size`: The buffer size of send and receive data for `ngx_http_fetch`, and the default value is recommended.

**Parameter Description of `queue_hash`**  

Field `queue_hash` is used to configure load balance, used for [`put`](../../api/ha/put.md) interface.


Use queue name to compute `hash` for load balance, if it configure to `on`.

* Advantage: it can ensure the sequence of fetched data is fixed, when storage node is working.
* Disadvantage: `QPS` is limited to single storage point, when write data to a specific queue.

Use default `round robin` for load balance, if it configured to `off`.

* Advantage: `QPS` is not limited to single storage point, when write data to a specific queue. 
* Disadvantage: the sequence of fetched data is not fixed.


**The default value is recommended expect the following fields.**

The meaning of most fields are consistent with nginx official configuration, for example:
* `keepalive_timeout`
* `keepalive`
* `proxy_connect_timeout`
* `proxy_send_timeout`
* `proxy_read_timeout`
* `proxy_buffer_size`

The configuration of `health_check` can refer to [`nginx_upstream_check_module`](https://github.com/yaoweibin/nginx_upstream_check_module)

### Configuration Generator###

Path: `hustmq/ha/nginx/conf/genconf.py`

After configure `nginx.json`, it generate configuration file `nginx.conf` by using `genconf.py`. The command is as followed:

    python genconf.py nginx.json

The content of `nginx.conf` is as followed:

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
                root /opt/huststore/hustmqha/html;
            }

            location /autost {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /stat_all {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /stat {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /put {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /get {
                hustmq_ha;
                #http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /ack {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /timeout {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /lock {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /max {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /purge {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /worker {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /evget {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /evsub {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /sub {
                hustmq_ha;
                #http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /pub {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /do_get {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /do_post {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /do_get_status {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
            }
            location /do_post_status {
                hustmq_ha;
                http_basic_auth_file /opt/huststore/hustmqha/conf/htpasswd;
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

### Common Problem ###

* How to disable `http basic authentication`?   
When configure `nginx.json`, delete the field of `"http_basic_auth_file"`. Use `genconf.py` to regenerate `nginx.conf`.

[Previous](index.md)

[Home](../../index.md)