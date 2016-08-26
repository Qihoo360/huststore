hustdb ha
--

First, install all the dependent common modules for `hustdb ha`:  

* [curl](https://github.com/curl/curl/releases)
* [zlog-1.2.12](https://github.com/HardySimpson/zlog/releases)
* [libevent-2.0.22-stable](https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz)
* [libevhtp-1.2.10](https://github.com/ellzey/libevhtp/releases)

Install `ha` and `sync server`：

    $ cd hustdb/ha/nginx
    $ ./configure --prefix=/data/hustdbha --add-module=src/addon
    $ make -j
    $ make install
    $ cd ../../sync
    $ make -j
    $ make install

Modify contents of `hustdb/ha/nginx/conf/nginx.json` as below ( **replace `backends` to your real `hustdb` machine list, at list two machine are required:**

    {
        "module": "hustdb_ha",
        "worker_processes": 4,
        "worker_connections": 1048576,
        "listen": 8082,
        "keepalive_timeout": 540,
        "keepalive": 32768,
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
            ["sync_passwd", "sync"]
        ],
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
                "/hustdb/zadd",
                "/hustdb/zrem",
                "/hustdb/zismember",
                "/hustdb/zscore",
                "/hustdb/zrangebyrank",
                "/hustdb/zrangebyscore",
                "/hustdb/stat",
                "/hustdb/stat_all",
                "/hustdb/file_count"
            ]
        }
    }

Execute `genconf.py` to generate `nginx.conf`, and replace the configuration:

    $ python genconf.py
    $ cp nginx.conf /data/hustdbha/conf/

Edit file `/data/hustdbha/conf/hustdbtable.json` as below(**replace `ip:port` of `val` to your real hustdb nodes**:

    {
        "table":
        [
            { "item": { "key": [0, 512], "val": ["192.168.1.101:9999", "192.168.1.102:9999"] } }
            { "item": { "key": [512, 1024], "val": ["192.168.1.102:9999", "192.168.1.101:9999"] } }
        ]
    }

After finish configuration, start `HA` and `sync server` in order:

    cd /data/hustdbha/sbin
    ./nginx
    cd /data/hustdbsync
    ./hustdbsync

Type in commands:

    curl -i -X GET 'localhost:8082/sync_alive'

We should be able to see the below infomation:

    HTTP/1.1 200 OK
    Server: nginx/1.9.4
    Date: Tue, 07 Jun 2016 03:25:18 GMT
    Content-Type: text/plain
    Content-Length: 3
    Connection: keep-alive
    
    ok

The result shows that the servers work as expected

[Previous page](index.md)

[Root directory](../index.md)