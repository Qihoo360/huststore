#ifndef __hustmq_ha_worker_def_20150710202507_h__
#define __hustmq_ha_worker_def_20150710202507_h__

#include "hustmq_ha_data_def.h"

typedef struct
{
	HustmqWorkerArray * arr;
	size_t size;
	size_t max_size;
} hustmq_workers_array_t;

void hustmq_ha_dispose_hustmq_workers_array(hustmq_workers_array_t * arr);

typedef struct
{
    char worker[HUSTMQ_HA_WORKER_SIZE + 1];
    int time;
    ngx_bool_t valid;
} hustmq_worker_item_t;

typedef struct
{
	c_dict_t(hustmq_worker_item_t *) dict; // worker as key
	size_t worker_items;
} hustmq_worker_dict_t;

void hustmq_ha_update_worker_dict(const hustmq_workers_array_t * arr, ngx_pool_t * pool, hustmq_worker_dict_t * worker_dict);
ngx_bool_t hustmq_ha_merge_worker_dict(hustmq_worker_dict_t * worker_dict, ngx_pool_t * pool, hustmq_worker_array_t * arr);

void hustmq_ha_init_worker_buffer(ngx_pool_t * pool);

#endif // __hustmq_ha_worker_def_20150710202507_h__
