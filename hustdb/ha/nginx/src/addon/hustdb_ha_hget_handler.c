#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_hget_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(false, false, hustdb_ha_check_tb, backend_uri, r);
}
