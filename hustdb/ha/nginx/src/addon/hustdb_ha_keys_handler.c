#include "hustdb_ha_handler.h"

static ngx_bool_t __check_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    enum { OFFSET = 0, SIZE, FILE, START, END };
    static const char * keys[] = { "offset", "size", "file", "start", "end" };
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
    if (!hustdb_ha_check_hash(value[FILE], value[START], value[END]))
    {
        return false;
    }
    if (!hustdb_ha_check_export(value[OFFSET], value[SIZE]))
    {
        return false;
    }
    return true;
}

ngx_int_t hustdb_ha_keys_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(__check_parameter, backend_uri, r);
}
