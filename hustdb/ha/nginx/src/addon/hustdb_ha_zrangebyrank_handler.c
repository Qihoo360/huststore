#include "hustdb_ha_handler_base.h"

ngx_int_t hustdb_ha_zrangebyrank_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_zread_keys_handler(NULL, backend_uri, r);
}
