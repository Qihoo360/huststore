#include "hustdb_ha_handler.h"
#include "hustdb_ha_write_handler.h"

ngx_int_t hustdb_ha_version_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static ngx_str_t version = ngx_string("hustdbha 1.7\n");
    r->headers_out.status = NGX_HTTP_OK;
    return ngx_http_send_response_imp(r->headers_out.status, &version, r);
}

ngx_int_t hustdb_ha_cache_append_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_del_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_exist_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_expire_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(hustdb_ha_has_ttl, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_hdel_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(hustdb_ha_has_tb, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_hexist_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, hustdb_ha_check_tb, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_hget_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, hustdb_ha_check_tb, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_hincrby_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(hustdb_ha_check_incr, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_hincrbyfloat_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(hustdb_ha_check_incr, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_hset_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(hustdb_ha_has_tb, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_persist_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_cache_handler(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_cache_ttl_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_del_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_handler(HUSTDB_METHOD_DEL, false, false, false, backend_uri, r, hustdb_ha_start_del);
}

ngx_int_t hustdb_ha_exist_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_file_count_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_get2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read2_handler(false, hustdb_ha_hash_peer_by_key, backend_uri, r);
}

ngx_int_t hustdb_ha_hdel_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_handler(HUSTDB_METHOD_HDEL, false, false, true, backend_uri, r, hustdb_ha_start_del);
}

ngx_int_t hustdb_ha_hexist_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, hustdb_ha_check_tb, backend_uri, r);
}

ngx_int_t hustdb_ha_hget_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, hustdb_ha_check_tb, backend_uri, r);
}

ngx_int_t hustdb_ha_hget2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read2_handler(false, hustdb_ha_hash_peer_by_key, backend_uri, r);
}

ngx_int_t hustdb_ha_hkeys_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(hustdb_ha_check_keys, backend_uri, r);
}

ngx_int_t hustdb_ha_hset_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_handler(HUSTDB_METHOD_HSET, false, false, true, backend_uri, r, hustdb_ha_start_post);
}

ngx_int_t hustdb_ha_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_handler(HUSTDB_METHOD_PUT, false, false, false, backend_uri, r, hustdb_ha_start_post);
}

ngx_int_t hustdb_ha_sadd_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_handler(HUSTDB_METHOD_SADD, true, true, true, backend_uri, r, hustdb_ha_start_post);
}

ngx_int_t hustdb_ha_sismember_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(true, true, hustdb_ha_check_tb, backend_uri, r);
}

ngx_int_t hustdb_ha_sismember2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read2_handler(true, hustdb_ha_hash_peer_by_body_key, backend_uri, r);
}

ngx_int_t hustdb_ha_smembers_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(hustdb_ha_check_keys, backend_uri, r);
}

ngx_int_t hustdb_ha_srem_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_write_handler(HUSTDB_METHOD_SREM, true, true, true, backend_uri, r, hustdb_ha_start_del);
}

ngx_int_t hustdb_ha_stat_all_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_zadd_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zwrite_handler(HUSTDB_METHOD_ZADD, backend_uri, r);
}

ngx_int_t hustdb_ha_zismember_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zread_handler(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_zrangebyrank_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zread_keys_handler(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_zrem_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zwrite_handler(HUSTDB_METHOD_ZREM, backend_uri, r);
}

ngx_int_t hustdb_ha_zscore_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zread_handler(NULL, backend_uri, r);
}

ngx_int_t hustdb_ha_zscore2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read2_handler(false, hustdb_ha_hash_peer_by_tb, backend_uri, r);
}
