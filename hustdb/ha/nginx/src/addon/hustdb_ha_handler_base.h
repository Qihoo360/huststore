#ifndef __hustdb_ha_handler_base_20150605184922_h__
#define __hustdb_ha_handler_base_20150605184922_h__

#include "hustdb_ha_table_def.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_subrequest_peer_t * peer;
    const char * key;
    const char * tb;
    uint64_t score;
    int8_t opt;
    ngx_str_t version;

    ngx_bool_t key_in_body;
    ngx_bool_t has_tb;
} hustdb_ha_ctx_t;

ngx_int_t hustdb_ha_send_response(
    ngx_uint_t status,
    const ngx_str_t * version,
    const ngx_str_t * response,
    ngx_http_request_t *r);

ngx_int_t hustdb_ha_on_subrequest_complete(ngx_http_request_t * r, void * data, ngx_int_t rc);

typedef ngx_bool_t (*hustdb_ha_check_parameter_t)(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_post_peer(
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r);

ngx_int_t hustdb_ha_read_handler(
    ngx_bool_t read_body,
    ngx_bool_t key_in_body,
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r);

ngx_int_t hustdb_ha_zread_handler(
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r);

ngx_int_t hustdb_ha_zread_keys_handler(
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r);

typedef ngx_http_subrequest_peer_t * (*hustdb_ha_hash_peer_t)(ngx_http_request_t *r);

ngx_http_subrequest_peer_t * hustdb_ha_hash_peer_by_key(ngx_http_request_t *r);
ngx_http_subrequest_peer_t * hustdb_ha_hash_peer_by_tb(ngx_http_request_t *r);
ngx_http_subrequest_peer_t * hustdb_ha_hash_peer_by_body_key(ngx_http_request_t *r);

ngx_int_t hustdb_ha_read2_handler(ngx_bool_t discard_body, hustdb_ha_hash_peer_t hashfunc, ngx_str_t * backend_uri, ngx_http_request_t *r);

ngx_bool_t hustdb_ha_init_identifier_cache(ngx_http_hustdb_ha_main_conf_t * mcf);

ngx_int_t hustdb_ha_fetch_sync_data(
    const ngx_str_t * http_uri,
    const ngx_str_t * http_args,
    const ngx_str_t * user,
    const ngx_str_t * passwd,
    ngx_http_upstream_rr_peer_t * sync_peer,
    ngx_http_request_t * r);

#endif // __hustdb_ha_handler_base_20150605184922_h__
