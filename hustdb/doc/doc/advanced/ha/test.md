测试
--

以下所有的脚本依赖于 [requests](https://github.com/request/request)。

### 自动化测试脚本 ###

路径：`hustdb/ha/nginx/test/autotest.py`

    usage:
        python autotest [uri] [action]
            [uri]
                uri: ip:port
            [action]
                put | get | del | exist |
                hset | hget | hdel | hexist |
                sadd | srem | sismember |
                zadd | zrem | zismember | zscore |
                stat_all | sync_status | get_table | loop
    sample:
        python autotest.py localhost:8082 loop
        python autotest.py localhost:8082 stat_all
        python autotest.py localhost:8082 sync_status

### 压力测试脚本 ###

路径：`hustdb/ha/nginx/mutitest.py`

    usage:
        python mutitest [number] [uri]
            number : number of process
            uri    : ip:port
    sample:
        python mutitest.py 2 localhost:8082

[上一级](../ha.md)

[根目录](../../index.md)