Intro
--

hustmq is distributed message queue With high performance, and consists of the following components:

* `hustmq`
* `hustmq ha`

The overall structure is as follows:
![architect](../../res/architect.png)

`hustmq ha` is distributed message queue component with high-availability, and it is a typical `nginx http` component.

Project Directory: `hustmq/ha`

This project consists of the following module:

* `nginx Module` : `hustmq/ha/nginx/src/addon`
* `Automated test scripts` : `hustmq/ha/nginx/test`

FAQ of nginx configuration, please refer to: 

> [nginx configuration: upstream](http://nginx.org/en/docs/http/ngx_http_upstream_module.html)

> [nginx configuration: proxy](http://nginx.org/en/docs/http/ngx_http_proxy_module.html)

[Home](../index.md)