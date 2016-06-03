#ifndef __ngx_http_fetch_upstream_20151225181127_h__
#define __ngx_http_fetch_upstream_20151225181127_h__

#include "ngx_http_fetch_utils.h"

ngx_int_t ngx_http_fetch_upstream_init(size_t cache_size, ngx_conf_t *cf);

ngx_http_request_t * ngx_http_fetch_create_request(
    size_t request_pool_size,
    ngx_log_t * log,
    ngx_uint_t variables,
    ngx_http_conf_ctx_t * ctx,
    ngx_cycle_t * cycle);

typedef struct
{
    const ngx_http_fetch_args_t * args;
    const ngx_http_auth_basic_key_t * auth;
    ngx_hash_t * headers_in_hash;
    ngx_str_t * schema;
    ngx_http_upstream_conf_t * umcf;
    ngx_http_request_t * r;
} ngx_http_fetch_upstream_data_t;

ngx_int_t ngx_http_fetch_init_upstream(
    const ngx_http_fetch_upstream_data_t * data);

#endif // __ngx_http_fetch_upstream_20151225181127_h__
