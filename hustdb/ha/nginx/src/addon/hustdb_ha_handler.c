#include "hustdb_ha_handler.h"

static ngx_str_t g_table_str = { 0, 0 };

ngx_bool_t hustdb_ha_init_table_str(const ngx_str_t * path, ngx_pool_t * pool)
{
    char * data = hustdb_ha_read_file((const char *)path->data, pool);
    if (!data)
    {
        return false;
    }
    ngx_str_t tmp = { strlen(data), (u_char *)data };
    g_table_str = tmp;
    return true;
}

ngx_int_t hustdb_ha_get_table_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return ngx_http_send_response_imp(NGX_HTTP_OK, &g_table_str, r);
}

static ngx_bool_t __check_keys_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    enum { OFFSET = 0, SIZE, FILE  };
    static const char * keys[] = { "offset", "size", "file"  };
    static int value[] = { 0, 0, 0, 0, 0 };
    static const size_t size = sizeof(keys) / sizeof(char *);
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        char * val = ngx_http_get_param_val(&r->args, keys[i], r->pool);
        if (!val)
        {
            return false;
        }
        value[i] = atoi(val);
    }
    if (!hustdb_ha_check_export(value[OFFSET], value[SIZE]))
    {
        return false;
    }
    return true;
}

ngx_int_t hustdb_ha_keys_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(__check_keys_parameter, backend_uri, r);
}

static ngx_bool_t __check_stat_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    char * tb = ngx_http_get_param_val(&r->args, "tb", r->pool);
    if (tb)
    {
        if (!hustdb_ha_check_key(tb))
        {
            return false;
        }
    }
    return true;
}

ngx_int_t hustdb_ha_stat_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(__check_stat_parameter, backend_uri, r);
}

ngx_int_t hustdb_ha_sync_alive_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = hustdb_ha_get_module_main_conf(r);
    if (!mcf)
    {
        return NGX_ERROR;
    }
    static ngx_str_t http_uri = ngx_string("/status.html");
    static ngx_str_t http_args = ngx_null_string;
    ngx_int_t rc = hustdb_ha_fetch_sync_data(&http_uri, &http_args,
        &mcf->sync_user, &mcf->sync_passwd, mcf->sync_peer, r);
    if (NGX_ERROR == rc)
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }
    return NGX_DONE;
}

ngx_int_t hustdb_ha_sync_status_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_hustdb_ha_main_conf_t * mcf = hustdb_ha_get_module_main_conf(r);
    if (!mcf)
    {
        return NGX_ERROR;
    }
    ngx_int_t rc = hustdb_ha_fetch_sync_data(&mcf->sync_status_uri, &mcf->sync_status_args,
        &mcf->sync_user, &mcf->sync_passwd, mcf->sync_peer, r);
    if (NGX_ERROR == rc)
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }
    return NGX_DONE;
}

static ngx_bool_t __check_zrangebyscore_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static const char * keys[] = { "min", "max" };
    static const size_t size = sizeof(keys) / sizeof(char *);
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        char * val = ngx_http_get_param_val(&r->args, keys[i], r->pool);
        if (!val)
        {
            return false;
        }
    }
    return hustdb_ha_check_keys(backend_uri, r);
}

ngx_int_t hustdb_ha_zrangebyscore_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zread_keys_handler(__check_zrangebyscore_parameter, backend_uri, r);
}

static ngx_str_t g_peer_count = { 0, 0 };

void hustdb_ha_init_peer_count(ngx_pool_t * pool)
{
    int count = (int) ngx_http_get_backend_count();
    g_peer_count.data = ngx_palloc(pool, 32);
    sprintf((char *)g_peer_count.data, "%d", count);
    g_peer_count.len = strlen((const char *)g_peer_count.data);
}

ngx_int_t hustdb_ha_peer_count_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return ngx_http_send_response_imp(NGX_HTTP_OK, &g_peer_count, r);
}
