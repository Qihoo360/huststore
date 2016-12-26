快速入门
--

### hustdb ###

安装 `hustdb`（需要sudo权限，用于安装libsnappy,libevhtp,libevent2.0）：

    $ cd hustdb/db/server/make/linux/
    $ sh build.sh

程序位置

* `hustdb/db/server/make/linux/hustdb`
* `hustdb/db/server/make/linux/hustdb.conf`

启动服务

    $ cd hustdb/db/server/make/linux/
    $ export LD_LIBRARY_PATH=/usr/local/lib
    $ ./hustdb

输入如下测试命令：

    curl -i -X GET 'localhost:8085/status.html'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	ok

返回该结果说明服务器工作正常。

更多细节，可以参考以下内容：

* [hustdb 部署细节](../advanced/hustdb/deploy.md)

### hustdb ha ###

首先安装 `hustdb ha` 所依赖的公共组件：  

* [zlog-1.2.12](https://github.com/HardySimpson/zlog/releases)
* [libevent-2.0.22-stable](https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz)
* [libevhtp-1.2.10](https://github.com/ellzey/libevhtp/releases)

安装 `pcre`：

    sudo yum install -y pcre-devel

编译 `libcurl`：

    $ cd third_party
    $ sh build_libcurl.sh

打开配置文件：

    $ cd ../hustdb/ha/nginx/conf/
    $ vi nginx.json

**替换 `backends` 为真实的 `hustdb` 机器列表，至少要有两个：**

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

运行 `genconf.py` 生成 `nginx.conf`：

    $ python genconf.py

编辑 `hosts` 文件：
    
    $ vi hosts

添加内容如下，**请替换为真实的 hustdb 节点**：

    192.168.1.101:9999
    192.168.1.102:9999

运行命令：

    python gen_table.py hosts hustdbtable.json

配置完毕之后，安装 `ha` 以及 `sync server`：

    $ cd ..
    $ chmod a+x configure
    $ sh Config.sh
    $ make -j
    $ make install
    $ cd ../../sync
    $ make -j
    $ make install

**先后** 启动 `HA` 以及 `sync server`：

    $ export LD_LIBRARY_PATH=/usr/local/lib
    $ /data/hustdbha/sbin/nginx
    $ cd /data/hustdbsync
    $ /data/hustdbsync/hustdbsync

输入如下测试命令：

    curl -i -X GET 'localhost:8082/version'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Fri, 16 Dec 2016 10:56:55 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive

    hustdbha 1.6

返回该结果说明服务器工作正常。

更多细节，可以参考以下内容：

* [hustdb ha 部署细节](../advanced/ha/deploy.md)
* [hustdb ha 配置细节](../advanced/ha/nginx.md)
* [hustdb ha 负载均衡表](../advanced/ha/table.md)
* [hustdb ha 日志配置](../advanced/ha/zlog.md)

[回首页](../index.md)