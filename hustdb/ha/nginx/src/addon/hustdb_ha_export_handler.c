#include "hustdb_ha_handler.h"

static ngx_bool_t __check_hash(ngx_http_request_t *r)
{
    enum { FILE = 0, START, END };
    static const char * keys[] = { "file", "start", "end" };
    static int value[] = { 0, 0, 0 };
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
    return true;
}

static ngx_bool_t __check_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    // TODO: you can check the parameter of request here
    char * tb = ngx_http_get_param_val(&r->args, "tb", r->pool);
    if (tb)
    {
        static const char * keys[] = { "file", "start", "end" };
        static const size_t size = sizeof(keys) / sizeof(char *);
        size_t i = 0;
        for (i = 0; i < size; ++i)
        {
            char * val = ngx_http_get_param_val(&r->args, keys[i], r->pool);
            if (val)
            {
                return false; // mutex with "tb" & "type"
            }
        }
        return hustdb_ha_check_key(tb);
    }
    return __check_hash(r);
}

ngx_int_t hustdb_ha_export_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(__check_parameter, backend_uri, r);
}
