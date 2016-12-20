#ifndef __ngx_http_utils_module_20150910210608_h__
#define __ngx_http_utils_module_20150910210608_h__

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <c_dict.h>

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define APPEND_MCF_ITEM(key, func) \
    { \
        ngx_string(key), \
        NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1, \
        func, \
        NGX_HTTP_SRV_CONF_OFFSET, \
        0, \
        NULL \
    }

typedef unsigned char ngx_bool_t;

typedef struct
{
    ngx_str_t uri;
    ngx_str_t backend_uri;
    ngx_int_t (*handler) (ngx_str_t * backend_uri, ngx_http_request_t *r);
} ngx_http_request_item_t;

ngx_str_t ngx_http_get_conf_path(ngx_cycle_t * cycle, ngx_str_t * name);
int ngx_http_get_flag_slot(ngx_conf_t * cf);
ngx_bool_t ngx_http_str_eq(const ngx_str_t * src, const ngx_str_t * dst);
ngx_str_t ngx_http_make_str(const ngx_str_t * str, ngx_pool_t * pool);
char * ngx_http_execute(const char * cmd, ngx_pool_t * pool);

ngx_str_t ngx_http_load_from_file(const char * path, ngx_pool_t * pool);
ngx_bool_t ngx_http_save_to_file(const char * data, const char * path);

typedef struct
{
    char * start;
    char * end;
} ngx_http_str_pos_t;

ngx_http_str_pos_t ngx_http_match_key(const ngx_str_t * args, const char * key, size_t key_size);
size_t ngx_http_get_buf_size(const ngx_buf_t * buf);
char * ngx_http_get_param_val(const ngx_str_t * args, const char * key, ngx_pool_t * pool);
ngx_str_t ngx_http_get_empty_str(ngx_pool_t * pool);
ngx_bool_t ngx_http_append_arg(const ngx_str_t * key, const ngx_str_t * val, ngx_http_request_t *r);
ngx_str_t ngx_http_remove_param(const ngx_str_t * args, const ngx_str_t * key, ngx_pool_t * pool);
ngx_bool_t ngx_http_check_key(ngx_http_request_t *r);

ngx_int_t ngx_http_send_response_imp(ngx_uint_t status, const ngx_str_t * response, ngx_http_request_t *r);
ngx_buf_t * ngx_http_get_request_body(ngx_http_request_t * r);
ngx_str_t * ngx_http_find_head_value(const ngx_list_t * headers, const ngx_str_t * key);
ngx_bool_t ngx_http_add_field_to_headers_out(const ngx_str_t * key, const ngx_str_t * val, ngx_http_request_t * r);
ngx_bool_t ngx_http_insert_head_to_body(const ngx_buf_t * head, ngx_http_request_t *r);
ngx_bool_t ngx_http_make_redirect(size_t location_size, const ngx_str_t * head, ngx_http_request_t * r);

typedef ngx_bool_t (*ngx_http_post_body_cb)(ngx_http_request_t * r, ngx_buf_t * buf, size_t buf_size);
void ngx_http_post_body_handler(ngx_http_request_t * r, ngx_http_post_body_cb cb);

typedef struct
{
    ngx_http_post_subrequest_t * subrequest;
    ngx_str_t uri;
    ngx_str_t args;
    ngx_str_t * backend_uri;
    ngx_str_t response;
} ngx_http_subrequest_ctx_t;

ngx_int_t ngx_http_run_subrequest(
        ngx_http_request_t *r,
        ngx_http_subrequest_ctx_t * ctx,
        ngx_http_upstream_rr_peer_t * peer);
ngx_int_t ngx_http_gen_subrequest(
        ngx_str_t * backend_uri,
        ngx_http_request_t *r,
        ngx_http_upstream_rr_peer_t * peer,
        ngx_http_subrequest_ctx_t * ctx,
        ngx_http_post_subrequest_pt handler);
ngx_int_t ngx_http_finish_subrequest(ngx_http_request_t * r);
ngx_int_t ngx_http_post_subrequest_handler(ngx_http_request_t * r, void * data, ngx_int_t rc);

ngx_bool_t ngx_http_peer_is_alive(ngx_http_upstream_rr_peer_t * peer);
ngx_http_upstream_rr_peer_t * ngx_http_first_peer(ngx_http_upstream_rr_peer_t * peer);
ngx_http_upstream_rr_peer_t * ngx_http_next_peer(ngx_http_upstream_rr_peer_t * peer);


typedef struct ngx_http_subrequest_peer_s ngx_http_subrequest_peer_t;

struct ngx_http_subrequest_peer_s
{
    ngx_http_upstream_rr_peer_t * peer;
    void * data;
    ngx_http_subrequest_peer_t * next;
};

ngx_http_subrequest_peer_t * ngx_http_init_peer_list(ngx_pool_t * pool, ngx_http_upstream_rr_peers_t * peers);
ngx_http_subrequest_peer_t * ngx_http_get_first_peer(ngx_http_subrequest_peer_t * peer);
ngx_http_subrequest_peer_t * ngx_http_get_next_peer(ngx_http_subrequest_peer_t * peer);
ngx_http_subrequest_peer_t * ngx_http_append_peer(
    ngx_pool_t * pool,
    ngx_http_upstream_rr_peer_t * peer,
    ngx_http_subrequest_peer_t * node);


typedef c_dict_t(ngx_http_upstream_rr_peer_t *) ngx_http_peer_dict_t;

ngx_http_upstream_rr_peer_t * ngx_http_peer_dict_get(ngx_http_peer_dict_t * dict, const char * key);
ngx_bool_t ngx_http_init_peer_dict(ngx_http_upstream_rr_peers_t * peers, ngx_http_peer_dict_t * peer_dict);

typedef struct
{
    ngx_http_upstream_rr_peer_t ** arr;
    size_t size;
} ngx_http_peer_array_t;

ngx_bool_t ngx_http_init_peer_array(
    ngx_pool_t * pool,
    ngx_http_upstream_rr_peers_t * peers,
    size_t size,
    ngx_http_peer_array_t * peer_array);
ngx_http_upstream_rr_peer_t * ngx_http_get_peer_by_index(ngx_http_peer_array_t * peer_array, size_t index);

typedef ngx_int_t (*ngx_http_addon_init_shm_ctx_t)(ngx_slab_pool_t * shpool, void * sh);
typedef struct
{
    size_t sizeof_sh;
    ngx_http_addon_init_shm_ctx_t init_sh;
    // in share memory
    void * sh;
    ngx_slab_pool_t * shpool;
} ngx_http_addon_shm_ctx_t;

ngx_shm_zone_t * ngx_http_addon_init_shm(
    ngx_conf_t * cf,
    ngx_str_t * name,
    size_t shm_size,
    size_t sizeof_sh,
    ngx_http_addon_init_shm_ctx_t init_sh,
    void * module);

#endif // __ngx_http_utils_module_20150910210608_h__
