Components
--

### `ngx_http_fetch_module` ###

Path: `hustmq/ha/nginx/src/http/modules`

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

This component provides `context-free` http interface, you can construct `ngx_http_request_t` object that is completely independent of any outer `ngx_http_request_t`, and send a http request to the `upstream` servers.

**Those who have experience in writing `subrequest`, should know how beneficial `context-free` could be. ^_^**

In general, if you want to construct a `subrequest`, you will have to config the reverse proxy interface of nginx, and the subrequest must come from client side. **Under such condition, a subrequest relies on the context of the parent.**

Under special circumstances, the subrequest you construct does not necessarily rely on parent's request. (e.g. There is only one timed task need when we need to periodically fetch data from upstream server), and `ngx_http_fetch_module` is born to solve problem like this.

Also, `ngx_http_fetch_module` provides interfaces which make the following code work properly: 

    for (i = 0; i < size; ++i)
    {
        ngx_http_fetch(args_list[i], auth);
    }

The design of `ngx_http_fetch_module` takes a lot of thinking from nginx, include:  

- `ngx_http_proxy_module.c`  
- `ngx_http_upstream_keepalive_module.c`
- `ngx_http_core_module.c`

Usage of `ngx_http_fetch_module` is simple, please check the this interface: 

    ngx_int_t ngx_http_fetch(const ngx_http_fetch_args_t * args, const ngx_http_auth_basic_key_t * auth);

See more details in `hustmq/ha/nginx/src/addon/hustmq_ha_fetch_stat.c`

[Previous](index.md)

[Home](../../index.md)