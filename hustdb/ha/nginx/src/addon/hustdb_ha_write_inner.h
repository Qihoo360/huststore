#ifndef __hustdb_ha_write_inner_20161013162328_h__
#define __hustdb_ha_write_inner_20161013162328_h__

#include "hustdb_ha_write_handler.h"

typedef enum
{
    STATE_WRITE_MASTER1,
    STATE_WRITE_MASTER2,
    STATE_WRITE_BINLOG
} hustdb_write_state_t;

typedef struct
{
    hustdb_ha_ctx_t base;
    hustdb_write_state_t state;
    int error_count;
    int skip_error_count; // for del
    ngx_http_subrequest_peer_t * error_peer;
    ngx_http_subrequest_peer_t * health_peer;
    uint32_t ttl;
} hustdb_ha_write_ctx_t;

hustdb_ha_write_ctx_t * hustdb_ha_create_write_ctx(ngx_http_request_t *r);
ngx_bool_t hustdb_ha_parse_args(ngx_bool_t has_tb, ngx_http_request_t *r, hustdb_ha_write_ctx_t * ctx);
ngx_bool_t hustdb_ha_parse_zset_args(ngx_http_request_t *r, hustdb_ha_write_ctx_t * ctx);
ngx_bool_t hustdb_ha_init_write_ctx_by_body(ngx_http_request_t *r, hustdb_ha_write_ctx_t * ctx);

typedef struct
{
    ngx_http_subrequest_peer_t * peer;
    ngx_http_subrequest_peer_t * error_peer;
    ngx_http_subrequest_peer_t * health_peer;
    int error_count;
    hustdb_write_state_t state;
} hustdb_ha_write_args_t;

ngx_bool_t hustdb_ha_init_write_args(const char * key, hustdb_ha_write_args_t * ctx);

ngx_int_t hustdb_ha_write_binlog(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx);

ngx_int_t hustdb_ha_write_sync_data(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx);

ngx_bool_t hustdb_ha_add_sync_head(const ngx_str_t * server, ngx_http_request_t *r);

#endif // __hustdb_ha_write_inner_20161013162328_h__
