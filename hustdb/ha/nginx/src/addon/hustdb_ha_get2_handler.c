#include "hustdb_ha_handler_base.h"

ngx_int_t hustdb_ha_get2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read2_handler("key", backend_uri, r);
}
