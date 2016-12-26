简介
--

hustdb 是一套高性能分布式存储解决方案。目前包含如下组件：

* `hustdb`
* `hustdb ha`

整体结构如下：  
![architect](../../res/architect.png)

### hustdb ###

高性能分布式存储服务

项目目录：`hustdb/db`

`hustdb` 的部署依赖于 `libevhtp`，其相关的配置可参考如下内容：
> [libevhtp](http://ellzey.github.io/libevhtp/)

### hustdb ha ###

`hustdb ha` 是保障分布式存储服务高可用性的组件，包含如下两个模块：  

* `HA` : `hustdb ha` 的核心组件，是一个典型的 `nginx http` 模块
* `sync server` : 数据同步服务

![ha](../../../../../res/ha.png)

`HA` 在写入后端 `db` 失败时，会将数据写入本地日志，`sync server` 负责将这些数据同步到后端 `db` 中。 `sync server` 在架构设计上只为 `HA` 提供服务， **对于用户而言是透明的** ，用户可以通过 `HA` 提供的接口查询 `sync server` 的信息。

项目目录：

* `hustdb/ha`
* `hustdb/sync`

项目包含的模块：

* `nginx 模块` : `hustdb/ha/nginx/src/addon`
* `libsync` 模块 : `hustdb/sync/module`
* `network` 模块 : `hustdb/sync/network`
* `自动化测试脚本`：`hustdb/ha/nginx/test`

`libsync` 模块依赖于 [libcurl](https://curl.haxx.se)  
`network` 模块依赖于 [libevhtp](https://github.com/ellzey/libevhtp)

对于 nginx 配置中的常见问题，可以参考如下内容：

> [nginx 配置之 upstream](http://nginx.org/en/docs/http/ngx_http_upstream_module.html)

> [nginx 配置之 proxy](http://nginx.org/en/docs/http/ngx_http_proxy_module.html)

`hustdb ha` 的部署依赖于 `zlog`，其相关的配置可参考如下内容：
> [zlog 用户手册](https://hardysimpson.github.io/zlog/UsersGuide-CN.html)

[回首页](../index.md)