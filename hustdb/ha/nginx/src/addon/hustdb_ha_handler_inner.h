#ifndef __hustdb_ha_handler_inner_20161013212422_h__
#define __hustdb_ha_handler_inner_20161013212422_h__

#include "hustdb_ha_handler_base.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
} hustdb_ha_loop_ctx_t;

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    const ngx_str_t * keys;
} hustdb_ha_peer_ctx_t;

typedef struct
{
    ngx_http_subrequest_peer_t * peer;
    size_t bucket;
    int bad_buckets;
} hustdb_ha_bucket_buf_t;

ngx_str_t * hustdb_ha_get_keys_from_header(ngx_http_request_t * r);
ngx_bool_t hustdb_ha_add_keys_to_header(const ngx_str_t * keys, ngx_http_request_t * r);
ngx_str_t * hustdb_ha_get_version(ngx_http_request_t * r);
ngx_bool_t hustdb_ha_add_version(const ngx_str_t * version, ngx_http_request_t * r);

#endif // __hustdb_ha_handler_inner_20161013212422_h__
