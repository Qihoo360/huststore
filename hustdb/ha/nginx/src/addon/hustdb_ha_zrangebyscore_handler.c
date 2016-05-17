#include "hustdb_ha_handler_base.h"

static ngx_bool_t __check_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
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
    return hustdb_ha_zread_keys_handler(__check_parameter, backend_uri, r);
}
