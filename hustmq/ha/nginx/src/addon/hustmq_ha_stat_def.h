#ifndef __hustmq_ha_stat_def_20150711135657_h__
#define __hustmq_ha_stat_def_20150711135657_h__

#include "hustmq_ha_data_def.h"

typedef struct
{
	ngx_str_t * host;
	HustmqHaMessageQueueArray arr;
} backend_stat_item_t;

typedef struct
{
	backend_stat_item_t * arr;
	size_t size;
	size_t max_size;
} backend_stat_array_t;

void hustmq_ha_dispose_backend_stat_array(backend_stat_array_t * arr);

typedef struct
{
    hustmq_ha_queue_base_t base;
	char queue[HUSTMQ_HA_QUEUE_SIZE + 1];
	const char * host;
} hustmq_ha_queue_item_t;

typedef struct
{
    c_dict_t(hustmq_ha_queue_item_t *) host_dict; // host as key, hustmq_ha_queue_item_t * as value
} hustmq_ha_queue_value_t;

hustmq_ha_queue_item_t * hustmq_ha_host_dict_get(hustmq_ha_queue_value_t * dict, const char * key);

typedef struct
{
	c_dict_t(hustmq_ha_queue_value_t *) dict; // queue as key, hustmq_ha_queue_value_t * as value
	size_t queue_items;
} hustmq_ha_queue_dict_t;

hustmq_ha_queue_value_t * hustmq_ha_queue_dict_get(hustmq_ha_queue_dict_t * dict, const char * key);

typedef struct
{
    hustmq_ha_queue_base_t item;
    hustmq_ha_queue_value_t * queue_val;
} hustmq_ha_queue_ctx_t;

void hustmq_ha_update_queue_dict(ngx_bool_t status_cache, const backend_stat_array_t * arr, ngx_pool_t * pool, hustmq_ha_queue_dict_t * queue_dict);
ngx_bool_t hustmq_ha_merge_queue_dict(
        hustmq_ha_queue_dict_t * queue_dict,
        ngx_pool_t * pool,
        hustmq_ha_message_queue_array_t * arr);
ngx_bool_t hustmq_ha_merge_queue_item(hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue, hustmq_ha_message_queue_item_t * item);

void hustmq_ha_init_queue_base(const hustmq_ha_queue_base_t * src, hustmq_ha_queue_base_t * des);
void hustmq_ha_merge_queue_base(const hustmq_ha_queue_base_t * src, hustmq_ha_queue_base_t * des);
typedef ngx_bool_t (*hustmq_ha_queue_item_handler)(const char * host, const hustmq_ha_queue_item_t *item, void * data);
ngx_bool_t hustmq_ha_traverse_host_dict(hustmq_ha_queue_value_t * queue_val, hustmq_ha_queue_item_handler handler, void * data);

ngx_int_t hustmq_ha_init_stat_buffer(ngx_http_hustmq_ha_main_conf_t * mcf);
hustmq_ha_queue_dict_t * hustmq_ha_get_queue_dict();

void hustmq_ha_init_pub_ref_dict();

#endif // __hustmq_ha_stat_def_20150711135657_h__
