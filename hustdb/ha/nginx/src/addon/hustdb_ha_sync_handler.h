#ifndef __hustdb_ha_sync_handler_20160603190301_h__
#define __hustdb_ha_sync_handler_20160603190301_h__

#include "hustdb_ha_handler.h"

ngx_int_t hustdb_ha_fetch_sync_data(
    const ngx_str_t * http_uri,
    const ngx_str_t * http_args,
    const ngx_str_t * user,
    const ngx_str_t * passwd,
    ngx_http_upstream_rr_peer_t * sync_peer,
    ngx_http_request_t * r);

#endif // __hustdb_ha_sync_handler_20160603190301_h__
