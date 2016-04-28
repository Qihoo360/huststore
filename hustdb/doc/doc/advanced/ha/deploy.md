部署
--

### 依赖 ###

`hustdb ha` 主要依赖于 `libsync` 以及 `zlog`，两者的部署方法请分别参考 [这里](../libsync/deploy.md) 以及 [这里](dep.md) 。

### 部署脚本样例 ###

以下是一个完整的 **一键远程部署脚本** ，供参考：

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
        libsync.tar.gz \
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
        cd /data/tmp/; \
        rm -rf nginx; \
        rm -f nginx.tar.gz; \
        tar -zxf libsync.tar.gz -C .; \
        cd /data/tmp/libsync/; \
        make -j; \
        cp libsync.so /data/hustdbha/sbin/; \
        cd /data/tmp/; \
        cp upgrade.sh /data/hustdbha/sbin/; \
        cd /data/hustdbha/html/; \
        rm -f status.html; \
        echo "ok" > status.html; \
        rm -rf libsync; \
        rm -f libsync.tar.gz; \
        '

    echo 'finish!'


### 限制 ###

* `nginx.tar.gz` 为打包后的 `hustdb ha` 模块，其源代码路径为：`hustdb/ha/nginx` 。

* `upgrade.sh` 为 `hustdb ha` 平滑升级脚本，具体内容参考[这里](upgrade.md) 。

* `libsync.tar.gz` 是打包后的 `libsync`，源代码路径为： `hustdb/sync/libsync` 。

* `nginx.tar.gz`、`upgrade.sh`、`libsync.tar.gz` 需要和部署脚本在同一目录下。

* 样例中 `192.168.1.101` 代表生产环境所属的线上机，实际部署的时候替换为真实的机器即可。

* 假设该脚本运行的机器位于 `192.168.1.100` ，则 `192.168.1.100` 需要与 `192.168.1.101` 建立 ssh 信任关系，操作账户为 `jobs`，实际部署时，将 `jobs` 替换为真实的账户即可。

可参考如下脚本来打包以上内容：

    #!/bin/sh
    cd hustdb/
    mkdir deploy
    cd ha/
    tar -zcf nginx.tar.gz nginx
    cp upgrade.sh nginx.tar.gz ../deploy/
    cd ../sync/
    tar -zcf libsync.tar.gz libsync
    cp libsync.tar.gz ../deploy/

[上一级](../ha.md)

[根目录](../../index.md)