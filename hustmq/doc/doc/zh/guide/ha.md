hustmq ha
--

安装 `hustmq ha` 以及 `libsync` ：

    $ cd hustmq/ha/nginx
    $ ./configure --prefix=/data/hustmqha --add-module=src/addon
    $ make -j
    $ make install

修改 `hustmq/ha/nginx/conf/nginx.json` 内容如下，其中 **`backends` 请替换为真实的 `hustmq` 机器列表，至少要有一个：**

    {
        "module": "hustmq_ha",
        "worker_connections": 1048576,
        "listen": 8080,
        "keepalive_timeout": 540,
        "keepalive": 32768,
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

运行 `genconf.py` 生成 `nginx.conf`，并替换配置：

    $ python genconf.py
    $ cp nginx.conf /data/hustmqha/conf/

配置完毕之后，启动 nginx：

    cd /data/hustmqha/sbin
    ./nginx

输入如下测试命令：

    curl -i -X GET 'localhost:8080/autost'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Tue, 01 Mar 2016 08:13:42 GMT
    Content-Type: text/plain
    Content-Length: 0
    Connection: keep-alive

返回该结果说明服务器工作正常。

[上一级](index.md)

[根目录](../index.md)