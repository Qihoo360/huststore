#ifndef __hustmq_ha_handler_filter_20150618053539_h__
#define __hustmq_ha_handler_filter_20150618053539_h__

#include "hustmq_ha_stat_def.h"

ngx_bool_t hustmq_ha_put_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue);
ngx_bool_t hustmq_ha_pre_get_check(const hustmq_ha_queue_base_t * item);
ngx_bool_t hustmq_ha_get_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue);
ngx_bool_t hustmq_ha_max_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue);

ngx_bool_t hustmq_ha_publish_queue_item_check(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue, hustmq_ha_queue_ctx_t * ctx);

ngx_bool_t hustmq_ha_idx_check(unsigned int idx, unsigned int si, unsigned int ci);

#endif // __hustmq_ha_handler_filter_20150618053539_h__
