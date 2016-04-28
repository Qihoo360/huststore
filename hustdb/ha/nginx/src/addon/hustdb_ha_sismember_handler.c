#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_sismember_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_read_handler(true, true, hustdb_ha_check_tb, backend_uri, r);
}
