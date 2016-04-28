#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_smembers_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(hustdb_ha_check_keys, backend_uri, r);
}
