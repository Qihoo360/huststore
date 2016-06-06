#ifndef __ngx_http_fetch_20151224133408_h__
#define __ngx_http_fetch_20151224133408_h__

#include "ngx_http_fetch_utils.h"

typedef struct
{
    size_t request_pool_size;
    size_t keepalive_cache_size;
    size_t connection_cache_size;
    ngx_conf_t * cf;
    ngx_http_upstream_rr_peers_t * peers;
} ngx_http_fetch_essential_conf_t;

typedef struct
{
    ngx_msec_t connect_timeout;
    ngx_msec_t send_timeout;
    ngx_msec_t read_timeout;
    ngx_msec_t timeout;
    size_t buffer_size;
    ngx_bufs_t bufs;
    size_t busy_buffers_size;
} ngx_http_fetch_upstream_conf_t;

typedef ngx_http_conf_port_t * (*ngx_http_fetch_select_port_t)(ngx_http_conf_port_t * arr, ngx_uint_t size);
typedef ngx_http_conf_addr_t * (*ngx_http_fetch_select_addr_t)(ngx_http_conf_addr_t * arr, ngx_uint_t size);

typedef struct
{
    ngx_http_fetch_select_port_t select_port;
    ngx_http_fetch_select_addr_t select_addr;
} ngx_http_fetch_selector_t;

typedef struct
{
    ngx_str_t * arr;
    size_t size;
} ngx_url_array_t;

// for example: if you need to fetch these urls: ["127.0.0.1:8081", "127.0.0.1:8082", "127.0.0.1:8083"],
//   which are NOT in "nginx.conf", then you can call this function to get upstream peers.
ngx_http_upstream_rr_peers_t  * ngx_http_init_upstream_rr_peers(const ngx_url_array_t * urls, ngx_conf_t * cf);

// selector can be set as NULL in most cases
ngx_int_t ngx_http_fetch_init_conf(
    const ngx_http_fetch_essential_conf_t * essential_conf,
    ngx_http_fetch_upstream_conf_t * uscf,
    ngx_http_fetch_selector_t * selector);

ngx_int_t ngx_http_fetch(const ngx_http_fetch_args_t * args, const ngx_http_auth_basic_key_t * auth);

#endif // __ngx_http_fetch_20151224133408_h__
