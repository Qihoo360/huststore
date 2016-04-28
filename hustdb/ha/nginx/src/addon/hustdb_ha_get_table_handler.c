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
