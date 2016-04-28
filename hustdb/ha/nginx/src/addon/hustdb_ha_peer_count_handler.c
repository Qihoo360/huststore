#include "hustdb_ha_handler.h"

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
