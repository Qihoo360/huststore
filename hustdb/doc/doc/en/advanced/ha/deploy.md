Deployment
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

`sync server` needs to `POST` data to the backend machines synchronizing. An open source library [libcurl](https://curl.haxx.se) is used for transproting data.

    Downloads: https://curl.haxx.se/download.html

    $ sudo yum install -y libidn-devel

    $ cd third_party
    $ tar -zxvf curl-curl-7_50_2.tar.gz
    $ cd curl-curl-7_50_2 
    $ ./buildconf
    $ ./configure --disable-ldap --disable-ldaps
    $ make

### libevent ###

`sync server` uses `libevent` as the event driven module in the network layer.

    $ wget https://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz
    $ tar -zxf libevent-2.0.22-stable.tar.gz
    $ cd libevent-2.0.22-stable
    $ ./configure
    $ make
    $ sudo make install

### libevhtp ###

`sync server` uses `libevhtp` as `http` framework. We need `cmake` to compile `libevhtp`.

    $ wget https://github.com/ellzey/libevhtp/archive/1.2.10.tar.gz -O libevhtp-1.2.10.tar.gz
    $ tar -zxf libevhtp-1.2.10.tar.gz
    $ cd libevhtp-1.2.10
    $ mkdir build
    $ cd build
    $ cmake ..
    $ make
    $ sudo make install

### Script example for Deployment ###

Below is a complete **one-key script for remote deployment**, it also includes deployment of `HA` and `sync server` for reference:

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
        huststore.tar.gz \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/tmp/; \
        rm -rf huststore; \
        tar -zxf huststore.tar.gz -C .; \
        cd /data/tmp/huststore/hustdb/ha/nginx; \
        sh Config.sh; \
        make -j; \
        make install; \
        cd /data/hustdbha/html/; \
        rm -f status.html; \
        echo "ok" > status.html; \
        cp /data/tmp/huststore/hustdb/ha/upgrade.sh /data/hustdbha/sbin/; \
        cd /data/tmp/huststore/third_party; \
        ./build_libcurl.sh; \
        cp /data/tmp/huststore/hustdb/sync; \
        make -j; \
        make install; \
        cp /data/tmp; \
        rm -rf huststore; \
        rm -f huststore.tar.gz; \
        '
    
    echo 'finish!'


### Restrictions ###

* `huststore.tar.gz` is the compressed package for the source code of `huststore`.

* `huststore.tar.gz` need to be in the same directory.  

* In the above example, `192.168.1.101` represents the online machine in production environment, replace it to your real ip in production environment.

* Suppose this script runs on `192.168.1.100`, then `192.168.1.100` needs to build SSH trust relationship (login passwordless) with `192.168.1.101`, operation account is `jobs`, replace it with your account in your real production environment.

### Script example for remote deployment of HA ###

If only `HA` needs to be re-deployed, you can refer to the below scripts: 

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
        huststore.tar.gz \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/tmp/; \
        rm -rf huststore; \
        tar -zxf huststore.tar.gz -C .; \
        cd /data/tmp/huststore/hustdb/ha/nginx; \
        sh Config.sh; \
        make -j; \
        make install; \
        cd /data/hustdbha/html/; \
        rm -f status.html; \
        echo "ok" > status.html; \
        cp /data/tmp/huststore/hustdb/ha/upgrade.sh /data/hustdbha/sbin/; \
        cd /data/tmp/; \
        rm -rf huststore; \
        rm -f huststore.tar.gz; \
        '
    
    echo 'finish!'

### Script example for remote deployment of sync server ###

If only `sync server` needs to be re-deployed, you can refer to the below scripts: 

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
        huststore.tar.gz \
        jobs@192.168.1.101:/data/tmp/
    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd /data/tmp/huststore/third_party; \
        ./build_libcurl.sh; \
        cp /data/tmp/huststore/hustdb/sync; \
        make -j; \
        make install; \
        cp /data/tmp; \
        rm -rf huststore; \
        rm -f huststore.tar.gz; \
        '
    
    echo 'finish!'

Make sure `sync server` has exit before executing the above script, command to exit `sync server` is:

    $ cd /data/hustdbsync
    $ export LD_LIBRARY_PATH=/usr/local/lib
    $ /data/hustdbsync/hustdbsync -q

Script for remote operations:

    echo '[192.168.1.101]...'

    sudo -u jobs \
        ssh -oStrictHostKeyChecking=no \
        jobs@192.168.1.101 \
        ' \
        cd  /data/hustdbsync; \
        export LD_LIBRARY_PATH=/usr/local/lib; \
        /data/hustdbsync/hustdbsync -q; \
        '
    
    echo 'finish!'

[Previous](../ha.md)

[Home](../../index.md)