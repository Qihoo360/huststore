hustdb ha
--
`hustdb ha` is a component to guarantee the high availability of distributed storage service, it is composed of two modules:   

* `HA` : a core module of `hustdb ha`, this is a typical `nginx http` module
* `sync server` :  this module will provide data synchronization service

![ha](../../../../../res/ha.png)

`HA` will write data to local log when it failed to write to backend `db`, `sync server` will synchronize these data to the backend `db`. In archtecture design, `sync server` provides service for `HA`, it is transparent to users. Users can use interfaces provided by `HA` to query information of `sync server`.

Project directories :

* `hustdb/ha`
* `hustdb/sync`

Project modules :

* `nginx` module : `hustdb/ha/nginx/src/addon`
* `libsync` module : `hustdb/sync/module`
* `network` module : `hustdb/sync/network`
* `automatic test script` : `hustdb/ha/nginx/test`

`libsync` module depends on [libcurl](https://curl.haxx.se)  
`network` module depends on [libevhtp](https://github.com/ellzey/libevhtp)

For common problems in nginx configuration, please refer to:

> [nginx upstream](http://nginx.org/en/docs/http/ngx_http_upstream_module.html)

> [nginx proxy](http://nginx.org/en/docs/http/ngx_http_proxy_module.html)

`hustdb ha` deployment depends on `zlog`, for detailed config on zlog, please refer to:  
> [zlog user manual](https://hardysimpson.github.io/zlog/UsersGuide-CN.html)

[Previous page](index.md)

[Home](../index.md)