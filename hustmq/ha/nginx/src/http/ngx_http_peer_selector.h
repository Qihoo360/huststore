#ifndef __ngx_http_peer_selector_20150602164134_h__
#define __ngx_http_peer_selector_20150602164134_h__

#include <ngx_config.h>
#include <ngx_core.h>
#include "ngx_http_upstream_round_robin.h"

ngx_http_upstream_rr_peers_t * ngx_http_get_backends();
size_t ngx_http_get_backend_count();

#endif // __ngx_http_peer_selector_20150602164134_h__
