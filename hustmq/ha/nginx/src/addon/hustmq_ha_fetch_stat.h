#ifndef __hustmq_ha_fetch_stat_20160223163211_h__
#define __hustmq_ha_fetch_stat_20160223163211_h__

#include "hustmq_ha_request_handler.h"

typedef void (*hustmq_ha_merge_backend_stats_t)(ngx_http_hustmq_ha_main_conf_t * conf, backend_stat_array_t * backend_stats);
typedef ngx_bool_t (*hustmq_ha_set_backend_stat_item_t)(ngx_http_request_t * r, backend_stat_item_t * it);

ngx_int_t hustmq_ha_init_fetch_cache(
    ngx_http_hustmq_ha_main_conf_t * mcf,
    hustmq_ha_set_backend_stat_item_t set,
    hustmq_ha_merge_backend_stats_t merge);
void hustmq_ha_invoke_autost(ngx_log_t * log);

#endif // __hustmq_ha_fetch_stat_20160223163211_h__
