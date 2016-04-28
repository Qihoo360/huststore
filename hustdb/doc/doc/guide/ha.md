hustdb ha
--

首先安装 `hustdb ha` 所依赖的公共组件：  

* [zlog](https://github.com/HardySimpson/zlog/releases)
* [curl](https://github.com/curl/curl/releases)

安装 `hustdb ha` 以及 `libsync` ：

    $ cd hustdb/ha/nginx
    $ ./configure --prefix=/data/hustdbha --add-module=src/addon
    $ make -j
    $ make install
    $ cd ../../sync/libsync
    $ make -j
    $ cp libsync.so /data/hustdbha/sbin/

修改 `hustdb/ha/nginx/conf/nginx.json` 内容如下，其中 **`backends` 请替换为真实的 `hustdb` 机器列表，至少要有两个：**

    {
        "module": "hustdb_ha",
        "worker_connections": 1048576,
        "listen": 8082,
        "keepalive_timeout": 540,
        "keepalive": 32768,
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
                "192.168.1.102:9999"
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

运行 `genconf.py` 生成 `nginx.conf`，并替换配置：

    $ python genconf.py
    $ cp nginx.conf /data/hustdbha/conf/

编辑 `/data/hustdbha/conf/hustdbtable.json` 内容如下，其中 `val` 所配置的 `ip:port` **请替换为真实的 hustdb 节点**：

    {
        "table":
        [
            { "item": { "key": [0, 1024], "val": ["192.168.1.101:9999", "192.168.1.102:9999"] } }
        ]
    }

配置完毕之后，启动 nginx：

    cd /data/hustdbha/sbin
    ./nginx

输入如下测试命令：

    curl -i -X GET 'localhost:8082/get_table'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Mon, 29 Feb 2016 08:02:52 GMT
    Content-Type: text/plain
    Content-Length: 624
    Connection: keep-alive
    
    {
        "table":
        [
            { "item": { "key": [0, 1024], "val": ["192.168.1.101:9999", "192.168.1.102:9999"] } }
        ]
    }

返回该结果说明服务器工作正常。

[上一级](index.md)

[根目录](../index.md)