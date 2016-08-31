Dependency
--

### zlog ###

`hustdb ha` will record failed write operation in local log files at runtime, these log files will be used by `sync server` for log synchronization, we adopt a high performance open source log library [zlog](http://hardysimpson.github.io/zlog/).

    Downloads: https://github.com/HardySimpson/zlog/releases

	$ tar -zxvf zlog-latest-stable.tar.gz
	$ cd zlog-latest-stable/
	$ make 
	$ sudo make install

	$ sudo vi /etc/ld.so.conf
	/usr/local/lib
	$ sudo ldconfig

### libcurl ###

`sync server` needs to `POST` data to the backend machines which are synchronizing. An open source library [libcurl](https://curl.haxx.se) is used for transporting data.

	Downloads: https://curl.haxx.se/download.html

	$ tar -zxvf curl-7.45.0.tar.gz
	$ cd curl-7.45.0
	$ make
	$ sudo make install

### libevent ###

`sync server` uses `libevent` as the event driven module in network layer

    $ wget https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz
    $ tar -zxf libevent-2.0.22-stable.tar.gz
    $ cd libevent-2.0.22-stable
    $ ./configure
    $ make
    $ sudo make install

### libevhtp ###

`sync server` uses `libevhtp` as `http` framework. You need `cmake` to compile `libevhtp`.

    $ wget https://github.com/ellzey/libevhtp/archive/1.2.10.tar.gz -O libevhtp-1.2.10.tar.gz
    $ tar -zxf libevhtp-1.2.10.tar.gz
    $ cd libevhtp-1.2.10
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ sudo make install

[Previous](../ha.md)

[Home](../../index.md)