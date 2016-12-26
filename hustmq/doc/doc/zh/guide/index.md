快速入门
--

### hustmq ###

安装 `hustdb`（需要sudo权限，用于安装libsnappy,libevhtp,libevent2.0）：

    $ cd hustdb/db/server/make/linux/
    $ sh build.sh

程序位置

* `hustdb/db/server/make/linux/hustdb`
* `hustdb/db/server/make/linux/hustdb.conf`

启动服务

    $ cd hustdb/db/server/make/linux/
    $ ./hustdb

输入如下测试命令：

    curl -i -X GET 'localhost:8085/status.html'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	ok

返回该结果说明服务器工作正常。

### hustmq ha ###

安装 `pcre`：

    sudo yum install -y pcre-devel

打开配置文件：

    $ cd hustmq/ha/nginx/conf/
    $ vi nginx.json

**将 `backends` 替换为真实的 `hustmq` 机器列表，至少要有一个：**

    {
        ......
        "proxy":
        {
            ......
            "backends": ["192.168.1.101:9999"],
            ......
        }
    }

运行 `genconf.py` 生成 `nginx.conf`：

    $ python genconf.py

配置完毕之后，安装 `hustmq ha`：

    $ cd hustmq/ha/nginx
    $ sh Config.sh
    $ make -j
    $ make install

启动 nginx：

    $ export LD_LIBRARY_PATH=/usr/local/lib
    $ /data/hustmqha/sbin/nginx

输入如下测试命令：

    curl -i -X GET 'localhost:8080/version'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Fri, 16 Dec 2016 10:54:47 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive

    hustmqha 1.6

返回该结果说明服务器工作正常。

[根目录](../index.md)