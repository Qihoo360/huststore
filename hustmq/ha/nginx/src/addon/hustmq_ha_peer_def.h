#ifndef __hustmq_ha_peer_def_20150923194528_h__
#define __hustmq_ha_peer_def_20150923194528_h__

#include "hustmq_ha_stat_def.h"

ngx_bool_t hustmq_ha_inconsistent(ngx_http_subrequest_peer_t * peer);
ngx_bool_t hustmq_ha_init_peer_dict(ngx_http_upstream_rr_peers_t * peers);
ngx_bool_t hustmq_ha_init_peer_list(ngx_pool_t * pool, ngx_http_upstream_rr_peers_t * peers);
ngx_bool_t hustmq_ha_init_hash(ngx_pool_t * pool, ngx_http_upstream_rr_peers_t * peers);
ngx_http_upstream_rr_peer_t * hustmq_ha_get_peer(ngx_str_t * queue);
ngx_str_t hustmq_ha_encode_ack_peer(ngx_str_t * peer_name, ngx_pool_t * pool);
ngx_http_upstream_rr_peer_t * hustmq_ha_decode_ack_peer(ngx_str_t * ack_peer, ngx_pool_t * pool);
ngx_http_subrequest_peer_t * hustmq_ha_build_pub_peer_list(hustmq_ha_queue_ctx_t * queue_ctx, ngx_pool_t * pool);
ngx_http_subrequest_peer_t * hustmq_ha_build_sub_peer_list(hustmq_ha_queue_value_t * queue_val, int idx, ngx_pool_t * pool);

#endif // __hustmq_ha_peer_def_20150923194528_h__
