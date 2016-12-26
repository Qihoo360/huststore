依赖
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

    $ sudo yum install -y libidn-devel

    $ cd third_party
    $ tar -zxvf curl-curl-7_50_2.tar.gz
    $ cd curl-curl-7_50_2 
    $ ./buildconf
    $ ./configure --disable-ldap --disable-ldaps
    $ make

### libevent ###

`sync server` 的网络层的事件模块由 `libevent` 提供。

    $ sudo yum install openssl-devel -y
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

[上一页](../ha.md)

[回首页](../../index.md)