#ifndef __hustdb_ha_utils_20150601202703_h__
#define __hustdb_ha_utils_20150601202703_h__

#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_addon_def.h>
#include <ngx_http_utils_module.h>
#include <ngx_http_fetch.h>
#include <c_dict.h>
#include "zlog.h"
#include "hustdb_ha_def.h"

#undef HUSTDB_TABLE_SIZE
#define HUSTDB_TABLE_SIZE    1024

#undef PEER_KEY
#define PEER_KEY             "peer"

typedef struct
{
    ngx_str_t * arr;
    size_t size;
} ngx_str_array_t;

typedef struct
{
    ngx_bool_t table_locked;
    ngx_queue_t cache_identifier;
    ngx_queue_t free_identifier;
} hustdb_ha_shctx_t;

typedef struct
{
    ngx_str_t public_pem_full_path;
    ngx_shm_zone_t * zone; // data => ngx_http_addon_shm_ctx_t => hustdb_ha_shctx_t
    ngx_http_upstream_rr_peer_t * sync_peer;
    ngx_str_t sync_status_args;
    // generated
    ngx_pool_t * pool;
    ngx_log_t * log;
    ngx_str_t prefix;
    ngx_bool_t debug_sync;
    ngx_str_t zlog_mdc;
    ngx_str_t hustdbtable_file;
    ngx_str_t hustdb_ha_shm_name;
    ssize_t hustdb_ha_shm_size;
    ngx_str_t public_pem;
    ngx_int_t identifier_cache_size;
    ngx_int_t identifier_timeout;
    ssize_t fetch_req_pool_size;
    ngx_int_t keepalive_cache_size;
    ngx_int_t connection_cache_size;
    ngx_int_t fetch_connect_timeout;
    ngx_int_t fetch_send_timeout;
    ngx_int_t fetch_read_timeout;
    ssize_t fetch_buffer_size;
    ngx_int_t sync_port;
    ngx_str_t sync_status_uri;
    ngx_str_t sync_user;
    ngx_str_t sync_passwd;
    ngx_str_t binlog_uri;
} ngx_http_hustdb_ha_main_conf_t;

void * hustdb_ha_get_module_main_conf(ngx_http_request_t * r);

ngx_http_upstream_rr_peers_t * ngx_http_get_backends();
size_t ngx_http_get_backend_count();

ngx_str_t hustdb_ha_make_str(ngx_str_t * val, ngx_http_request_t * r);
ngx_str_t hustdb_ha_strcat(const ngx_str_t * str, int num, ngx_pool_t * pool);

void hustdb_ha_init_peer_count(ngx_pool_t * pool);

typedef struct
{
    const char ** peers;
    size_t size;
} hustdb_ha_peers_t;

ngx_bool_t hustdb_ha_init_peer_array(ngx_pool_t * pool);
ngx_http_upstream_rr_peer_t * hustdb_ha_get_peer(ngx_http_request_t *r);

ngx_bool_t hustdb_ha_init_log_dirs(const ngx_str_t * prefix, ngx_pool_t * pool);
ngx_bool_t hustdb_ha_init_table_str(const ngx_str_t * path, ngx_pool_t * pool);

ngx_bool_t hustdb_ha_write_log(
    const ngx_str_t * zlog_mdc,
    const ngx_str_t * uri,
    ngx_buf_t * buf,
    ngx_pool_t * pool);

ngx_bool_t hustdb_ha_check_key(const char * key);
ngx_bool_t hustdb_ha_check_export(int bucket_offset, int size);
ngx_bool_t hustdb_ha_check_hash(int file, int start, int end);

ngx_bool_t hustdb_ha_check_tb(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_bool_t hustdb_ha_check_keys(ngx_str_t * backend_uri, ngx_http_request_t *r);

char * hustdb_ha_get_key_from_body(ngx_http_request_t * r);

char * hustdb_ha_read_file(const char * path, ngx_pool_t * pool);

ngx_bool_t hustdb_ha_lock_table(ngx_http_hustdb_ha_main_conf_t * mcf);
void hustdb_ha_unlock_table();

ngx_str_t hustdb_ha_rsa_encrypt(const ngx_str_t * plain, const char * public_pem_file, ngx_pool_t * pool);

typedef ngx_bool_t (*hustdb_ha_traverse_line_cb)(const char * line, ngx_pool_t * pool, void * data);
ngx_bool_t hustdb_ha_incr_count(const char * line, ngx_pool_t * pool, void * data);
ngx_bool_t hustdb_ha_append_item(const char * line, ngx_pool_t * pool, void * data);

ngx_bool_t hustdb_ha_overwrite_backends(const ngx_str_array_t * backends, ngx_pool_t * pool);

#endif // __hustdb_ha_utils_20150601202703_h__
