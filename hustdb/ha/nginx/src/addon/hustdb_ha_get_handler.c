#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, NULL, backend_uri, r);
}
