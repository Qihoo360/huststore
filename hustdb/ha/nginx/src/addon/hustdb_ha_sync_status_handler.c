#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_sync_status_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    char * json_val = hustdb_ha_get_status();
    if (!json_val)
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }

    size_t len = strlen(json_val);

    ngx_str_t response;
    response.data = ngx_palloc(r->pool, len);
    response.len = len;

    memcpy(response.data, json_val, len);

    hustdb_ha_dispose_status(json_val);

    r->headers_out.status = NGX_HTTP_OK;
    return ngx_http_send_response_imp(r->headers_out.status, &response, r);
}
