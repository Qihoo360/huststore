hustmq ha
--

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

    $ export LD_LIBRARY_PATH=/usr/local/lib
    $ /data/hustmqha/sbin/nginx

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

[Previous](index.md)

[Home](../index.md)