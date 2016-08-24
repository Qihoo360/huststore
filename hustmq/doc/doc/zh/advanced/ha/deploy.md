部署
--

### 部署脚本样例 ###

以下是一个完整的 **一键远程部署脚本** ，供参考：

    #!/bin/sh
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
        sh Config.sh; \
        make -j; \
        make install; \
        cd /data/tmp/; \
        rm -rf nginx; \
        rm -f nginx.tar.gz; \
        cd /data/tmp/; \
        cp upgrade.sh /data/hustmqha/sbin/; \
        cd /data/hustmqha/html/; \
        rm -f status.html; \
        echo "ok" > status.html; \
        '

    echo 'finish!'


### 限制 ###

* `nginx.tar.gz` 为打包后的 `hustmq ha` 模块，其源代码路径为：`hustmq/ha/nginx` 。

* `upgrade.sh` 为 `hustmq ha` 平滑升级脚本，具体内容参考[这里](upgrade.md) 。

* `nginx.tar.gz`、`upgrade.sh` 需要和部署脚本在同一目录下。

* 样例中 `192.168.1.101` 代表生产环境所属的线上机，实际部署的时候替换为真实的机器即可。

* 假设该脚本运行的机器位于 `192.168.1.100` ，则 `192.168.1.100` 需要与 `192.168.1.101` 建立 ssh 信任关系，操作账户为 `jobs`，实际部署时，将 `jobs` 替换为真实的账户即可。

可参考如下脚本来打包以上内容：

    #!/bin/sh
    cd hustmq/
    mkdir deploy
    cd ha/
    tar -zcf nginx.tar.gz nginx
    cp upgrade.sh nginx.tar.gz ../deploy/

[上一级](index.md)

[根目录](../../index.md)