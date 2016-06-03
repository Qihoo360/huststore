#include "hustdb_ha_sync_handler.h"

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
