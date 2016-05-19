#ifndef __hustdb_ha_handler_20150601202210_h__
#define __hustdb_ha_handler_20150601202210_h__

#include "hustdb_ha_handler_base.h"

ngx_int_t hustdb_ha_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_get2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_exist_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_del_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_keys_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_hset_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_hget_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_hget2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_hdel_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_hexist_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_hkeys_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_sadd_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_srem_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_sismember_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_smembers_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_file_count_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_stat_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_stat_all_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_peer_count_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_sync_status_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_get_table_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_set_table_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_zismember_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_zscore_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_zscore2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_zadd_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_zrem_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_zrangebyrank_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);
ngx_int_t hustdb_ha_zrangebyscore_handler(ngx_str_t * backend_uri, ngx_http_request_t *r);

#endif // __hustdb_ha_handler_20150601202210_h__
