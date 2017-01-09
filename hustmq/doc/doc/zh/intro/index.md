简介
--

hustmq 是一套高性能分布式消息队列。目前包含如下组件：

* `hustmq`
* `hustmq ha`

整体结构如下：
![architect](../../res/architect.png)

`hustmq ha` 是保障分布式消息队列高可用性的组件，该组件是一个典型的 `nginx http` 模块。

项目目录：`hustmq/ha`

项目包含的模块：

* `nginx 模块` : `hustmq/ha/nginx/src/addon`
* `自动化测试脚本`：`hustmq/ha/nginx/test`

对于 nginx 配置中的常见问题，可以参考如下内容：

> [nginx 配置之 upstream](http://nginx.org/en/docs/http/ngx_http_upstream_module.html)

> [nginx 配置之 proxy](http://nginx.org/en/docs/http/ngx_http_proxy_module.html)

[回首页](../index.md)