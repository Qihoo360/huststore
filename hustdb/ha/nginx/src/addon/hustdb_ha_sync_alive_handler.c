#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_sync_alive_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_str_t response = ngx_string("ok");
    r->headers_out.status = NGX_HTTP_OK;
    return ngx_http_send_response_imp(r->headers_out.status, &response, r);
}