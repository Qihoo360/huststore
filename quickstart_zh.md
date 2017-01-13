<h1 id="id_top">快速入门</h1>

* [快速试用](#id_try)
* [更多](#id_adv)
    * [脚本工具](#id_adv_tools)
    * [预备工作](#id_adv_prepare)
    * [安装第三方依赖](#id_adv_dep)
    * [hustdb 集群部署](#id_adv_hustdb_cluster)
    * [hustmq 集群部署](#id_adv_hustmq_cluster)
    * [总结](#id_adv_summary)
* [附录](#id_appendix)

<h2 id="id_try">快速试用</h2>

这一部分展示了 `huststore` 在 **单机** 上的部署试用过程，不含集群的部署内容。整个过程只有 `hustdb` 一个组件会被用到，其他的模块用不上。集群的试用请参考[这里](#id_adv)。

    $ sudo yum groupinstall -y 'Development tools'
    $ sudo yum install -y pcre-devel libidn-devel openssl-devel

    $ wget https://github.com/Qihoo360/huststore/archive/v1.7.tar.gz -O huststore-1.7.tar.gz
    $ tar -zxf huststore-1.7.tar.gz
    $ cd huststore-1.7
    $ sh prebuild.sh --prefix=/opt/huststore
    $ sudo mkdir /opt/huststore
    $ sudo chown -R $USER:$USER /opt/huststore
    $ sh build.sh --module=3rd,hustdb

**警告：请确保目录 `/opt/huststore` 下有足够的磁盘空间，否则请换一个路径。**

启动服务：

    $ cd /opt/huststore/hustdb
    $ sh start.sh

测试服务：

    curl -i -X GET 'localhost:8085/status.html'

返回如下信息：

    HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	ok

停止服务：

    $ cd /opt/huststore/hustdb
    $ sh stop.sh

更多 API 请参考[这里](hustdb/doc/doc/zh/api/hustdb/hustdb.md)

[回顶部](#id_top)

<h2 id="id_adv">更多</h2>

这一部分展示了 `huststore` **集群** 的部署流程（非单机），集群的部署方式推荐在生产环境中使用。

* 请 **逐节** 阅读以下的部署流程，**不要跳过任何部分**。
* **最少准备两台机器** 来部署 huststore，例如： `["192.168.1.101", "192.168.1.102"]`。  
* **强烈推荐使用一台独立的机器完成构建过程** ，例如：`192.168.1.100` 。  

**相关约定**

* `user`: `jobs`
* `build machine` : `192.168.1.100`  
* `deployment machines` : `["192.168.1.101", "192.168.1.102"]`
* `hustdb`: `["192.168.1.101:8085", "192.168.1.102:8085"]`
* `hustdb ha`: `["192.168.1.101:8082", "192.168.1.102:8082"]`
* `hustmq`: `["192.168.1.101:8086", "192.168.1.102:8086"]`
* `hustmq ha`: `["192.168.1.101:8080", "192.168.1.102:8080"]`

**请用生产环境中真实的数据替换上面的相关参数（`user`, `build machine`, `deployment machines`）。**

<h3 id="id_adv_tools">脚本工具</h3>

**备注：以下提到的所有的工具都在项目的根目录下。**

#### prebuild.sh ####

`prebuild.sh` 用于准备 `huststore` 的构建环境，用法： 

    usage:
        sh prebuild.sh [option]
        
        [option]
            --help                             show the manual
            --prefix=PATH                      set installation prefix of 3rd & huststore
                
    sample:
        sh prebuild.sh --help
        sh prebuild.sh --prefix=/opt/huststore
        sh prebuild.sh

**备注：如果未设置 `--prefix` ， `prebuild.sh` 将使用 `/opt/huststore` 作为默认的安装路径。请确保该目录下有足够的磁盘空间**

[回顶部](#id_top)

#### build.sh ####

**备注：你需要运行 `prebuild.sh` 来生成 `build.sh`。**

`build.sh` 用于项目的构建并为生成相关模块的安装包（方便运维），用法:

    usage:
        sh build.sh [option]

        [option]
            --help                             show the manual

            --module=3rd                       build and generate installation package for third-party libs.
                                               It should be installed FIRST to build other modules.

            --module=hustdb                    build and generate installation package for hustdb
            --module=hustdbha                  build and generate installation package for hustdbha
            --module=hustmq                    build and generate installation package for hustmq
            --module=hustmqha                  build and generate installation package for hustmqha
            
            --clean                            clean obj & bin files of latest build
                
    sample:
        sh build.sh --help
        sh build.sh --module=hustdb
        sh build.sh --module=3rd,hustdb
        sh build.sh --module=3rd,hustdb,hustdbha
        sh build.sh --module=3rd,hustmq,hustmqha
        
        sh build.sh --clean

        sh build.sh

备注：如果 `--module` 未设置， `build.sh` 将默认构建 `huststore` 的**所有模块**，并生成如下的安装包：

    elf_3rd.tar.gz
    elf_hustdb.tar.gz
    elf_hustdbha.tar.gz
    elf_hustmq.tar.gz
    elf_hustmqha.tar.gz

这些安装包位于 `prebuild.sh` 的同级目录，可通过 `scp` & `tar` 命令安装到生产环境的 `/opt/huststore` 目录中。

如果 `--module` 被指定为特定的模块，例如 `hustdb`，那么将只生成 `elf_hustdb.tar.gz`。

警告：在构建其他模块之前，**你需要先构建并安装第三方的依赖**：  
    
    $ sh build.sh --module=3rd

[回顶部](#id_top)

#### remote_scp.py ####

`remote_scp.py` 是 `scp` 命令的一个包装，用于支持 **批量执行 `scp`**。

    usage:
        python remote_scp.py [option] [user] [host_file] [remote_folder] [local_file1] [local_file2] ...
        
        [option]
            --silent                          run in silent mode

    sample:
        python remote_scp.py jobs host.txt /opt/huststore/hustdbha/conf nginx.conf hustdbtable.json
        python remote_scp.py --silent jobs host.txt /opt/huststore/hustdbha/conf nginx.conf hustdbtable.json

[回顶部](#id_top)

#### remote_ssh.py ####

`remote_ssh.py` 是 `ssh` 命令的一个包装，用于支持 **批量执行 `ssh`**。

    usage:
        python remote_ssh.py [option] [user] [host_file] [cmds_file]
        
        [option]
            --silent                          run in silent mode
            
    sample:
        python remote_ssh.py jobs host.txt cmds.txt
        python remote_ssh.py --silent jobs host.txt cmds.txt

[回顶部](#id_top)

#### remote_prefix.py ####

`remote_prefix.py` 用于检查并设置远程机器的安装路径，用法：

    usage:
        python remote_prefix.py [user] [host_file] [prefix] [owner]
    sample:
        python remote_prefix.py jobs host.txt /opt/huststore jobs

参数：

* `user` : `ssh` 命令使用的用户名
* `host_file` : 记录远程机器列表的文件名
* `prefix` : 安装目录
* `owner` : **安装目录的所有者**

例如，当执行以下命令：

    python remote_prefix.py admin host.txt /opt/huststore jobs

`remote_prefix.py` 将以用户名 `admin` 逐台登陆 `host.txt` 所包含的远程机器列表，远程创建目录 `/opt/huststore`（如果该目录不存在） ，并将其所有者更换为用户 `jobs`。

出于便利，你可以将以下内容添加到远程机器的 `/etc/sudoers` 文件中：

    admin ALL = (ALL) ALL

    %jobs ALL=(jobs) ALL
    User_Alias USERS = %jobs
    USERS ALL= (jobs) NOPASSWD:ALL
    USERS ALL= NOPASSWD: /bin/su - jobs

#### remote_deploy.py ####

`remote_prefix.py` 用于将安装包一键部署至远程机器中，用法：

    usage:
        python remote_deploy.py [user] [host_file] [prefix] [tar]
    sample:
        python remote_deploy.py jobs host.txt /opt/huststore elf_hustdb.tar.gz

参数：

* `user` : `ssh` & `scp` 命令用到的用户名
* `host_file` : 记录远程机器列表的文件名
* `prefix` : **远程机器**的安装目录
* `tar` : **本地**的 `elf` 安装包

例如，当执行以下命令：

    python remote_deploy.py jobs host.txt /opt/huststore elf_hustdb.tar.gz

`remote_deploy.py` 将以用户名 `jobs` 逐台登陆 `host.txt` 所包含的远程机器列表，将本地安装包 `elf_hustdb.tar.gz` 拷贝至远程机器上，并 **解压** 至目录 `/opt/huststore` 。

#### remote_service.py ####

`remote_service.py` 用于控制远程机器上安装的 `huststore` 的服务，用法：

    usage:
        python remote_service.py [user] [host_file] [bin_folder] [action]
        
        [action]
            --start                           start remote service
            --stop                            stop remote service
            
    sample:
        python remote_service.py jobs host.txt /opt/huststore/hustdb --start
        python remote_service.py jobs host.txt /opt/huststore/hustdb --stop

参数：

* `user` : `ssh` 命令用到的用户名
* `host_file` : 记录远程机器列表的文件名
* `bin_folder` : 远程机器上安装的 `huststore` 相关服务的二进制文件所在的目录
* `action` : 启动/停止服务

[回顶部](#id_top)

#### make_conf.py ####

`make_conf.py` 用于快速生成 `hustdb ha` 或 `hustmq ha` 的配置，用法：

    usage:
        python make_conf.py [host_file] [module] [HA port] [backend port]
        
        [module]
            hustdbha
            hustmqha

    sample:
        python make_conf.py host.txt hustdbha 8082 8085
        python make_conf.py host.txt hustmqha 8080 8086

参数：

* `host_file` : 记录远程 `hustdb` | `hustmq` 机器列表的文件名
* `module` : `hustdbha` | `hustmqha`
* `HA port` : `hustdbha` | `hustmqha` 的监听端口
* `backend port` : `hustdb` | `hustmq` 的监听端口

[回顶部](#id_top)

<h3 id="id_adv_prepare">预备工作</h3>

登陆到 **构建机器** （`192.168.1.100`），下载 `huststore` 的源代码包，解压：

    $ ssh 192.168.1.100  # 请将ip替换为你的构建机器
    $ wget https://github.com/Qihoo360/huststore/archive/v1.7.tar.gz -O huststore-1.7.tar.gz
    $ tar -zxf huststore-1.7.tar.gz
    $ cd huststore-1.7

编辑 `hosts`：  

    $ vi hosts

添加如下内容并保存，**请替换为真实的部署机器的地址**：

    192.168.1.101
    192.168.1.102

运行 `prebuild.sh` 准备构建环境：

    $ sh prebuild.sh --prefix=/opt/huststore
    $ sudo mkdir /opt/huststore
    $ sudo chown -R $USER:$USER /opt/huststore

**警告：请确保目录 `/opt/huststore` 下有足够的磁盘空间，否则请换一个路径。**

运行 `remote_prefix.py` 批量设置远程机器的安装路径：

    $ python remote_prefix.py jobs hosts /opt/huststore jobs

[回顶部](#id_top)

<h3 id="id_adv_dep">安装第三方依赖</h3>

构建第三方库并生成安装包：

    $ sudo yum groupinstall -y 'Development tools'
    $ sudo yum install -y pcre-devel libidn-devel openssl-devel
    $ sh build.sh --module=3rd

安装第三方依赖（将安装包拷贝至远程机器，并解压至目录 `/opt/huststore`）：

    $ python remote_deploy.py jobs hosts /opt/huststore elf_3rd.tar.gz

[回顶部](#id_top)

<h3 id="id_adv_hustdb_cluster">hustdb 集群部署</h3>

#### hustdb ####

构建并生成 `hustdb` 的安装包： 

    $ sh build.sh --module=hustdb

部署 `hustdb`（将安装包拷贝至远程机器，并解压至目录 `/opt/huststore`）：

    $ python remote_deploy.py jobs hosts /opt/huststore elf_hustdb.tar.gz

启动服务（`ssh` 到每台远程机器，运行脚本 `/opt/huststore/hustdb/start.sh`）：

    $ python remote_service.py jobs hosts /opt/huststore/hustdb --start

输入如下测试命令：

    curl -i -X GET '192.168.1.101:8085/status.html'
    curl -i -X GET '192.168.1.102:8085/status.html'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	ok

返回该结果说明服务器工作正常。

[回顶部](#id_top)

#### hustdb ha ####

构建并生成 `hustdb ha` 的安装包： 

    $ sh build.sh --module=hustdbha

生成 `hustdb ha` 的配置： 

    $ python make_conf.py hosts hustdbha 8082 8085
    $ cp hustdb/ha/nginx/conf/nginx.conf nginx.conf.db
    $ cp hustdb/ha/nginx/conf/hustdbtable.json .

部署 `hustdb ha`（将安装包拷贝至远程机器，并解压至目录 `/opt/huststore`，然后替换配置文件）：

    $ python remote_deploy.py jobs hosts /opt/huststore elf_hustdbha.tar.gz
    $ cp nginx.conf.db nginx.conf
    $ python remote_scp.py --silent jobs hosts /opt/huststore/hustdbha/conf nginx.conf hustdbtable.json
    $ rm -f nginx.conf

启动服务（`ssh` 到每台远程机器，运行脚本 `/opt/huststore/hustdbha/sbin/start.sh` 和 `/opt/huststore/hustdbsync/start.sh`）：

    $ python remote_service.py jobs hosts /opt/huststore/hustdbha/sbin --start
    $ python remote_service.py jobs hosts /opt/huststore/hustdbsync --start

输入如下测试命令：

    curl -i -X GET '192.168.1.101:8082/version'
    curl -i -X GET '192.168.1.102:8082/version'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Fri, 16 Dec 2016 10:56:55 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive

    hustdbha 1.7

返回该结果说明服务器工作正常。

[回顶部](#id_top)

<h3 id="id_adv_hustmq_cluster">hustmq 集群部署</h3>

#### hustmq ####

构建并生成 `hustmq` 的安装包： 

    $ sh build.sh --module=hustmq

部署 `hustmq`（将安装包拷贝至远程机器，并解压至目录 `/opt/huststore`）：

    $ python remote_deploy.py jobs hosts /opt/huststore elf_hustmq.tar.gz

启动服务（`ssh` 到每台远程机器，运行脚本 `/opt/huststore/hustmq/start.sh`）：

    $ python remote_service.py jobs hosts /opt/huststore/hustmq --start

输入如下测试命令：

    curl -i -X GET '192.168.1.101:8086/status.html'
    curl -i -X GET '192.168.1.102:8086/status.html'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
	Content-Length: 3
	Content-Type: text/plain

	ok

返回该结果说明服务器工作正常。

[回顶部](#id_top)

#### hustmq ha ####

构建并生成 `hustmq ha` 的安装包： 

    $ sh build.sh --module=hustmqha

生成 `hustmq ha` 的配置： 

    $ python make_conf.py hosts hustdbha 8080 8086
    $ cp hustmq/ha/nginx/conf/nginx.conf nginx.conf.mq

部署 `hustmq ha`（将安装包拷贝至远程机器，并解压至目录 `/opt/huststore`，然后替换配置文件）：

    $ python remote_deploy.py jobs hosts /opt/huststore elf_hustmqha.tar.gz
    $ cp nginx.conf.mq nginx.conf
    $ python remote_scp.py --silent jobs hosts /opt/huststore/hustmqha/conf nginx.conf
    $ rm -f nginx.conf

启动服务（`ssh` 到每台远程机器，运行脚本 `/opt/huststore/hustmqha/sbin/start.sh`）：

    $ python remote_service.py jobs hosts /opt/huststore/hustmqha/sbin --start

输入如下测试命令：

    curl -i -X GET '192.168.1.101:8080/version'
    curl -i -X GET '192.168.1.102:8080/version'

可以看到服务器返回如下内容：

    HTTP/1.1 200 OK
    Server: nginx/1.10.0
    Date: Fri, 16 Dec 2016 10:54:47 GMT
    Content-Type: text/plain
    Content-Length: 13
    Connection: keep-alive

    hustmqha 1.7

返回该结果说明服务器工作正常。

[回顶部](#id_top)

<h3 id="id_adv_summary">总结</h3>

以上流程的整合：

    # ssh to build machine

    wget https://github.com/Qihoo360/huststore/archive/v1.7.tar.gz -O huststore-1.7.tar.gz
    tar -zxf huststore-1.7.tar.gz
    cd huststore-1.7

    # edit hosts
    # vi hosts
    # 192.168.1.101
    # 192.168.1.102

    # prebuild
    sh prebuild.sh --prefix=/opt/huststore
    python remote_prefix.py jobs hosts /opt/huststore jobs

    # build & make installation packages
    sh build.sh
    python make_conf.py hosts hustdbha 8082 8085
    cp hustdb/ha/nginx/conf/nginx.conf nginx.conf.db
    cp hustdb/ha/nginx/conf/hustdbtable.json .
    python make_conf.py hosts hustmqha 8080 8086
    cp hustmq/ha/nginx/conf/nginx.conf nginx.conf.mq


    # deploy 3rd
    python remote_deploy.py jobs hosts /opt/huststore elf_3rd.tar.gz

    # deploy huststore
    python remote_deploy.py jobs hosts /opt/huststore elf_hustdb.tar.gz
    python remote_deploy.py jobs hosts /opt/huststore elf_hustmq.tar.gz

    python remote_deploy.py jobs hosts /opt/huststore elf_hustdbha.tar.gz
    cp nginx.conf.db nginx.conf
    python remote_scp.py --silent jobs hosts /opt/huststore/hustdbha/conf nginx.conf hustdbtable.json
    rm -f nginx.conf

    python remote_deploy.py jobs hosts /opt/huststore elf_hustmqha.tar.gz
    cp nginx.conf.mq nginx.conf
    python remote_scp.py --silent jobs hosts /opt/huststore/hustmqha/conf nginx.conf
    rm -f nginx.conf

    # start huststore service
    python remote_service.py jobs hosts /opt/huststore/hustdb --start
    python remote_service.py jobs hosts /opt/huststore/hustmq --start
    python remote_service.py jobs hosts /opt/huststore/hustdbha/sbin --start
    python remote_service.py jobs hosts /opt/huststore/hustdbsync --start
    python remote_service.py jobs hosts /opt/huststore/hustmqha/sbin --start

[回顶部](#id_top)

<h2 id="id_appendix">附录</h2>

* `hustdb`
    * [hustdb 配置细节](hustdb/doc/doc/zh/advanced/hustdb.md)
* `hustdb ha`
    * [hustdb ha 部署细节](hustdb/doc/doc/zh/advanced/ha/deploy.md)
    * [hustdb ha 配置细节](hustdb/doc/doc/zh/advanced/ha/nginx.md)
    * [hustdb ha 负载均衡表](hustdb/doc/doc/zh/advanced/ha/table.md)
    * [hustdb ha 日志配置](hustdb/doc/doc/zh/advanced/ha/zlog.md)
* `hustmq`
    * [hustmq 配置细节](hustmq/doc/doc/zh/advanced/hustmq/index.md)
* `hustmq ha`
    * [hustmq ha 配置细节](hustmq/doc/doc/zh/advanced/ha/nginx.md)
    * [hustmq ha 部署细节](hustmq/doc/doc/zh/advanced/ha/deploy.md)
* `API`
    * [`hustdb`](hustdb/doc/doc/zh/api/hustdb/hustdb.md)
    * [`hustdb ha`](hustdb/doc/doc/zh/api/ha.md)
    * [`hustmq`](hustmq/doc/doc/zh/api/hustmq.md)
    * [`hustmq ha`](hustmq/doc/doc/zh/api/ha.md)

[回顶部](#id_top)

[回首页](README_ZH.md)