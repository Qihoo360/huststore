Deployment
--

### Example of Deployment Script ###

The following is complete **Automatic Remote Deployment Script**, just for refer:

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


### Restriction ###

* `nginx.tar.gz` is packed `hustmq ha` module, the path of source code: `hustmq/ha/nginx`.

* `upgrade.sh` is smooth upgrade script of `hustmq ha`, and please refer to [Here](upgrade.md).

* `nginx.tar.gz` and `upgrade.sh` needed to deploy on the same directory.

* In the example, `192.168.1.101` represent online machine, it can replaced with your machine for your deployment.

* Assume that the script is run on the machine `192.168.1.100`, then `192.168.1.100` needs establish ssl trust relation with `192.168.1.101` with user `jobs`. It can replace `jobs` with your account for your deployment.

It can refer to the following script to pack the above content.

    #!/bin/sh
    cd hustmq/
    mkdir deploy
    cd ha/
    tar -zcf nginx.tar.gz nginx
    cp upgrade.sh nginx.tar.gz ../deploy/

[Pervious](index.md)

[Home](../../index.md)