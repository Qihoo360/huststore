#include "hustdb_ha_write_handler.h"

ngx_int_t hustdb_ha_zadd_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zwrite_handler(HUSTDB_METHOD_ZADD, backend_uri, r);
}
