hustdb ha
--
`hustdb ha` 是保障分布式存储服务高可用性的组件，该组件是一个典型的 `nginx http` 模块。

项目目录：`hustdb/ha`

项目包含的模块：

* `nginx 模块` : `hustdb/ha/nginx/src/addon`
* `自动化测试脚本`：`hustdb/ha/nginx/test`

对于 nginx 配置中的常见问题，可以参考如下内容：

> [nginx 配置之 upstream](http://nginx.org/en/docs/http/ngx_http_upstream_module.html)

> [nginx 配置之 proxy](http://nginx.org/en/docs/http/ngx_http_proxy_module.html)

`hustdb ha` 的部署依赖于 `zlog`，其相关的配置可参考如下内容：
> [zlog 用户手册](https://hardysimpson.github.io/zlog/UsersGuide-CN.html)

[上一级](index.md)

[根目录](../index.md)