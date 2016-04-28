#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_stat_all_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustdb_ha_post_peer(NULL, backend_uri, r);
}
