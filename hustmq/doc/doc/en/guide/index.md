Getting Started
--

### hustmq ###

Install `hustdb`(need sudo, used for libsnappy, libevhtp, libevent2.0): 

    $ cd hustdb/db/server/make/linux/
    $ sh build.sh

Target path

* `hustdb/db/server/make/linux/hustdb`
* `hustdb/db/server/make/linux/hustdb.conf`

Start service

    $ cd hustdb/db/server/make/linux/
    $ ./hustdb

Type in the below command to test:

    curl -i -X GET 'localhost:8085/status.html'

Infomation returned:

    HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	ok

The result shows that the servers work as expected.

For detailed deployment and configuration, please check:

* [hustmq deployment](../advanced/hustmq/index.md)

### hustmq ha ###

Install `pcre`：

    sudo yum install -y pcre-devel

Open the configuration:  

    $ cd hustmq/ha/nginx/conf/
    $ vi nginx.json

**Replace `backends` to your real `hustdb` machine list, at least one is required:**

    {
        ......
        "proxy":
        {
            ......
            "backends": ["192.168.1.101:9999"],
            ......
        }
    }

Run `genconf.py` to generate `nginx.conf`:

    $ python genconf.py

After configuration, install `hustmq ha`:

    $ cd hustmq/ha/nginx
    $ sh Config.sh
    $ make -j
    $ make install

Start nginx:

    $ export LD_LIBRARY_PATH=/opt/huststore/3rd/lib
    $ /opt/huststore/hustmqha/sbin/nginx

Input the following test command:

    curl -i -X GET 'localhost:8080/version'

Then server will output the following information:

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Fri, 16 Dec 2016 10:54:47 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive

    hustmqha 1.6

Server works just fine if the above result is returned.

For detailed deployment and configuration, please check:

* [hustdb ha configuration](../advanced/ha/nginx.md)
* [hustdb ha deployment](../advanced/ha/deploy.md)

[Home](../index.md)