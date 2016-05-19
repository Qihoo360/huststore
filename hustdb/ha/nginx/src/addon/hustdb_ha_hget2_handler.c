#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_hget2_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read2_handler("key", backend_uri, r);
}
