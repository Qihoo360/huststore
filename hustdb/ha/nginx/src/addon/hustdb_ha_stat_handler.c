#include "hustdb_ha_handler.h"

static ngx_bool_t __check_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    // TODO: you can check the parameter of request here
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
    return hustdb_ha_post_peer(__check_parameter, backend_uri, r);
}
