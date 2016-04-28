测试
--

以下所有的脚本依赖于 [requests](https://github.com/request/request)。

### 自动化测试脚本 ###

路径：`hustdb/ha/nginx/test/autotest.py`

    usage:
        python autotest [option] [action]
            [option]
                host: ip or domain
            [action]
                stat_all | sync_status | get_table | export
                put | get | del | exist | print | clean
                hset | hget | hdel | hexist
                sadd | srem | sismember
                loop | bloop | loopput | loopbput | loopstatus \ tb_loop
    sample:
        python autotest.py loop
        python autotest.py stat_all
        python autotest.py sync_status
        python autotest.py print
        python autotest.py clean
        python autotest.py 192.168.1.101 loop
        python autotest.py 192.168.1.101 print
        python autotest.py 192.168.1.101 clean

### 压力测试脚本 ###

路径：`hustdb/ha/nginx/mutitest.py`

    usage:
        mutitest [number] [host]
            [number]
                required: number of process
            [host]
                optional: test host name
    sample:
        python mutitest.py 2
        python mutitest.py 2 192.168.1.101

[上一级](../ha.md)

[根目录](../../index.md)