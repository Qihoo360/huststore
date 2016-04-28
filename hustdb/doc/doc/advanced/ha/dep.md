依赖
--

### zlog ###

`hustdb ha` 运行时会将写入失败的数据记录为本地日志，供 `libsync` 模块进行同步。日志功能使用了开源的高性能日志服务模块 [zlog](http://hardysimpson.github.io/zlog/) 。

    Downloads: https://github.com/HardySimpson/zlog/releases

	$ tar -zxvf zlog-latest-stable.tar.gz
	$ cd zlog-latest-stable/
	$ make 
	$ sudo make install

	$ sudo vi /etc/ld.so.conf
	/usr/local/lib
	$ sudo ldconfig

### libsync ###

路径：`hustdb/sync/libsync`

安装完 `hustdb ha` 之后，需要安装 `libsync` 模块才能启用数据同步功能。假定 `hustdb ha` 的安装目录为 `/data/hustdbha` ， 则 `libsync` 的部署流程如下：

    $ cd hustdb/sync/libsync/
    $ make
    $ cp libsync.so /data/hustdbha/sbin/

[上一级](../ha.md)

[根目录](../../index.md)