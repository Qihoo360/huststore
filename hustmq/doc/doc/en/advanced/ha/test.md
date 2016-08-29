Test
--

All of the following scripts rely on [requests](https://github.com/request/request).  

### Automated test script ###

Path: `hustmq/ha/nginx/test/autotest.py`

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

**Notes**

`evget` and `evput` need to be tested in pair, you can start them separately in two terminals. (Same process for `evsub` and `evpub`) 

    # terminal 1
    $ python autotest.py evget
    # terminal 2
    $ python autotest.py evput

[Previous](index.md)

[Home](../../index.md)