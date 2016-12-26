测试
--

以下所有的脚本依赖于 [requests](https://github.com/request/request)。

### 自动化测试脚本 ###

路径：`hustmq/ha/nginx/test/autotest.py`

    usage:
        python autotest [option] [action]
            [option]
                host: ip or domain
            [action]
                loop
                stat_all
                bput
                bget
                evget
                evput
                evsub
                evpub
                worker
                purge
    sample:
        python autotest.py loop
        python autotest.py stat_all
        python autotest.py 192.168.1.101 stat_all

**备注**

`evget` 和 `evput` 需要成对配合进行测试，具体操作可以分别在两个终端上开启脚本（`evsub` 和 `evpub` 同理）

    # terminal 1
    $ python autotest.py evget
    # terminal 2
    $ python autotest.py evput

[上一页](index.md)

[回首页](../../index.md)