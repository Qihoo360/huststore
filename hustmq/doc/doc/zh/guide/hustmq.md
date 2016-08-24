hustmq
--

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

[上一级](index.md)

[根目录](../index.md)