组件
--

### `ngx_http_fetch_module` ###

路径：`hustmq/ha/nginx/src/http/modules`

* `ngx_http_fetch_utils.h`
* `ngx_http_fetch_utils.c`
* `ngx_http_fetch_cache.h`
* `ngx_http_fetch_cache.c`
* `ngx_http_fetch_encode.h`
* `ngx_http_fetch_encode.c`
* `ngx_http_fetch_decode.h`
* `ngx_http_fetch_decode.c`
* `ngx_http_fetch_keepalive.h`
* `ngx_http_fetch_keepalive.c`
* `ngx_http_fetch_upstream_handler.h`
* `ngx_http_fetch_upstream_handler.c`
* `ngx_http_fetch_upstream.h`
* `ngx_http_fetch_upstream.c`
* `ngx_http_fetch.h`
* `ngx_http_fetch.c`
* `ngx_http_fetch_module.c`

该模块提供上下文无关的 http 访问接口（`context-free`），你可以在不依赖于任何外部 `ngx_http_request_t` 的条件下构造一个完全独立的 `ngx_http_request_t` 对象，并向上游主机发起 http 请求（`upstream`）。

**所有编写过 `subrequest` 代码的朋友，想必都了解这一点（`context-free`）意味着什么 ^_^**

通常情况下，如果你想构造一个 `subrequest` ，你必须配置 nginx 反向代理的接口，并且要由客户端来驱动该子请求的产生。 **在这种设计下，一个子请求的生成必须依赖于一个父请求的上下文。**

但是在特殊情况下，你所构造的子请求并不需要依赖于父请求（例如，周期性地向上游主机抓取数据，这通常开一个定时任务即可）， `ngx_http_fetch_module` 正是为解决此类问题而出现。

此外， `ngx_http_fetch_module` 提供的接口使得编写如下形式的代码成为可能：

    for (i = 0; i < size; ++i)
    {
        ngx_http_fetch(args_list[i], auth);
    }

`ngx_http_fetch_module` 的设计大量参考了 nginx 源代码，包括：

- `ngx_http_proxy_module.c`  
- `ngx_http_upstream_keepalive_module.c`
- `ngx_http_core_module.c`

`ngx_http_fetch_module` 的使用非常简单，只需要了解如下接口：

    ngx_int_t ngx_http_fetch(const ngx_http_fetch_args_t * args, const ngx_http_auth_basic_key_t * auth);

具体的使用方法可以参考 `hustmq/ha/nginx/src/addon/hustmq_ha_fetch_stat.c`

[上一级](index.md)

[回首页](../../index.md)