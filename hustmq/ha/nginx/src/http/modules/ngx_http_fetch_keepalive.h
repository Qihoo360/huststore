#ifndef __ngx_http_fetch_keepalive_20151225170155_h__
#define __ngx_http_fetch_keepalive_20151225170155_h__

#include "ngx_http_fetch_utils.h"

ngx_int_t ngx_http_fetch_keepalive_init(size_t keepalive, ngx_conf_t *cf);
ngx_int_t ngx_http_fetch_init_keepalive_peer(
    ngx_http_fetch_addr_t * addr,
    ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us);

#endif // __ngx_http_fetch_keepalive_20151225170155_h__
