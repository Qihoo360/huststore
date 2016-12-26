hustdb ha
--

First, install all the dependent common modules for `hustdb ha`:  

* [zlog-1.2.12](https://github.com/HardySimpson/zlog/releases)
* [libevent-2.0.22-stable](https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz)
* [libevhtp-1.2.10](https://github.com/ellzey/libevhtp/releases)

Install `pcre`：

    sudo yum install -y pcre-devel

Build `libcurl`：

    $ cd third_party
    $ sh build_libcurl.sh

Open the configuration:  

    $ cd ../hustdb/ha/nginx/conf/
    $ vi nginx.json

**Replace `backends` to your real `hustdb` machine list, at list two machine are required:**

    {
        ......
        "proxy":
        {
            ......
            "backends": 
            [
                "192.168.1.101:9999", 
                "192.168.1.102:9999"
            ],
            ......
        }
    }

Execute `genconf.py` to generate `nginx.conf`:

    $ python genconf.py

Edit file `hosts`:  

    $ vi hosts

Add contents as below and save, **please replace to your real hustdb nodes**：

    192.168.1.101:9999
    192.168.1.102:9999

Execute command：

    python gen_table.py hosts hustdbtable.json

After finish configuration, install `ha` and `sync server`:  

    $ cd ..
    $ chmod a+x configure
    $ sh Config.sh
    $ make -j
    $ make install
    $ cd ../../sync
    $ make -j
    $ make install

Start `HA` and `sync server` **in order**:

    $ export LD_LIBRARY_PATH=/usr/local/lib
    $ /data/hustdbha/sbin/nginx
    $ cd /data/hustdbsync
    $ /data/hustdbsync/hustdbsync

Type in commands:

    curl -i -X GET 'localhost:8082/version'

We should be able to see the below infomation:

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Fri, 16 Dec 2016 10:56:55 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive

    hustdbha 1.6

The result shows that the servers work as expected.

[Previous](index.md)

[Home](../index.md)