部署
--

### zlog ###

`hustdb ha` 运行时会将写入失败的数据记录为本地日志，供 `sync server` 进行同步。日志功能使用了开源的高性能日志服务模块 [zlog](http://hardysimpson.github.io/zlog/) 。

    Downloads: https://github.com/HardySimpson/zlog/releases

	$ tar -zxvf zlog-latest-stable.tar.gz
	$ cd zlog-latest-stable/
	$ make 
	$ sudo make install

	$ sudo vi /etc/ld.so.conf
	/usr/local/lib
	$ sudo ldconfig

### libcurl ###

`sync server` 同步时需要将数据 `POST` 到后端机。这里使用了开源的 `url` 传输库 [libcurl](https://curl.haxx.se)。

	Downloads: https://curl.haxx.se/download.html

	$ tar -zxvf curl-7.45.0.tar.gz
	$ cd curl-7.45.0
	$ make
	$ sudo make install

### libevent ###

`sync server` 的网络层的事件模块由 `libevent` 提供。

    $ wget https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz
    $ tar -zxf libevent-2.0.22-stable.tar.gz
    $ cd libevent-2.0.22-stable
    $ ./configure
    $ make
    $ sudo make install

### libevhtp ###

`sync server` 的网络层的 `http` 框架由 `libevhtp` 提供。编译 `libevhtp` 需要使用 `cmake`。

    $ wget https://github.com/ellzey/libevhtp/archive/1.2.10.tar.gz -O libevhtp-1.2.10.tar.gz
    $ tar -zxf libevhtp-1.2.10.tar.gz
    $ cd libevhtp-1.2.10
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ sudo make install
### 部署脚本样例 ###

以下是一个完整的 **一键远程部署脚本** ，同时包含 `HA` 以及 `sync server` 的部署，供参考：

    #!/bin/bash
    echo '[192.168.1.101]...'
    
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/; \
        test -d tmp || mkdir -p tmp; \
        '
    sudo -u jobs \
        scp -oStrictHostKeyChecking=no \
        nginx.tar.gz \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        scp -oStrictHostKeyChecking=no \
        sync.tar.gz \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        scp -oStrictHostKeyChecking=no \
        upgrade.sh \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/tmp/; \
        rm -rf nginx; \
        tar -zxf nginx.tar.gz -C .; \
        cd /data/tmp/nginx/; \
        ./configure --prefix=/data/hustdbha --add-module=src/addon; \
        make -j; \
        make install; \
        cd /data/hustdbha/html/; \
        rm -f status.html; \
        echo "ok" > status.html; \
        cd /data/tmp/; \
        cp upgrade.sh /data/hustdbha/sbin/; \
        rm -rf nginx; \
        rm -f nginx.tar.gz; \
        cd /data/tmp/; \
        tar -zxf sync.tar.gz -C .; \
        cd /data/tmp/sync/; \
        make -j; \
        make install; \
        rm -rf sync; \
        rm -f sync.tar.gz; \
        '
    
    echo 'finish!'


### 限制 ###

* `nginx.tar.gz` 为打包后的 `hustdb ha` 模块，其源代码路径为：`hustdb/ha/nginx` 。

* `upgrade.sh` 为 `hustdb ha` 平滑升级脚本，具体内容参考[这里](upgrade.md) 。

* `sync.tar.gz` 是打包后的 `sync server`，源代码路径为： `hustdb/sync` 。

* `nginx.tar.gz`、`upgrade.sh`、`sync.tar.gz` 需要和部署脚本在同一目录下。

* 样例中 `192.168.1.101` 代表生产环境所属的线上机，实际部署的时候替换为真实的机器即可。

* 假设该脚本运行的机器位于 `192.168.1.100` ，则 `192.168.1.100` 需要与 `192.168.1.101` 建立 ssh 信任关系，操作账户为 `jobs`，实际部署时，将 `jobs` 替换为真实的账户即可。

可参考如下脚本来打包以上内容：

    #!/bin/sh
    cd hustdb/
    mkdir deploy
    cd ha/
    tar -zcf nginx.tar.gz nginx
    cp upgrade.sh nginx.tar.gz ../deploy/
    cd ../
    tar -zcf sync.tar.gz sync
    cp sync.tar.gz deploy/

### 远程部署 HA 脚本范例 ###

如果仅仅只需要重新部署 `HA` ，可以参考如下脚本：

    #!/bin/bash
    echo '[192.168.1.101]...'
    
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/; \
        test -d tmp || mkdir -p tmp; \
        '
    sudo -u jobs \
        scp -oStrictHostKeyChecking=no \
        nginx.tar.gz \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        scp -oStrictHostKeyChecking=no \
        upgrade.sh \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/tmp/; \
        rm -rf nginx; \
        tar -zxf nginx.tar.gz -C .; \
        cd /data/tmp/nginx/; \
        ./configure --prefix=/data/hustdbha --add-module=src/addon; \
        make -j; \
        make install; \
        cd /data/hustdbha/html/; \
        rm -f status.html; \
        echo "ok" > status.html; \
        cd /data/tmp/; \
        cp upgrade.sh /data/hustdbha/sbin/; \
        rm -rf nginx; \
        rm -f nginx.tar.gz; \
        '
    
    echo 'finish!'

### 远程部署 sync server 脚本范例 ###

如果仅仅只需要重新部署 `sync server` ，可以参考如下脚本：

    echo '[192.168.1.101]...'
    
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/; \
        test -d tmp || mkdir -p tmp; \
        '
    sudo -u jobs \
        scp -oStrictHostKeyChecking=no \
        sync.tar.gz \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/tmp/; \
        tar -zxf sync.tar.gz -C .; \
        cd /data/tmp/sync/; \
        make -j; \
        make install; \
        rm -rf sync; \
        rm -f sync.tar.gz; \
        '
    
    echo 'finish!'

在执行上述脚本之前，请先确保 `sync server` 进程已经退出。退出 `sync server` 的命令为：

    $ cd /data/hustdbsync
    $ ./hustdbsync -q

远程操作的脚本如下：

    echo '[192.168.1.101]...'

    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd  /data/hustdbsync && ./hustdbsync -q; \
        '
    
    echo 'finish!'

[上一级](../ha.md)

[根目录](../../index.md)