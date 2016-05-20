#ifndef __hustmq_ha_data_def_20150605185936_h__
#define __hustmq_ha_data_def_20150605185936_h__

#include "hustmq_ha_utils.h"

typedef struct
{
	char * buf;
	size_t items;
	size_t max_size;
} hustmq_ha_buffer_t;

typedef struct
{
    unsigned int start_idx;
    unsigned int consistent_idx;
    time_t timestamp;
} hustmq_ha_idx_t;

typedef struct
{
    hustmq_ha_idx_t idx;
    int ready[HUSTMQ_HA_READY_SIZE];
    hustmq_queue_type_t type;
    int max;
    int lock;
    int unacked;
    int timeout;
    ngx_bool_t valid;
} hustmq_ha_queue_base_t;

typedef struct
{
    hustmq_ha_queue_base_t base;
	const char * queue;
} hustmq_ha_message_queue_item_t;

typedef struct
{
	hustmq_ha_message_queue_item_t * arr;
    size_t size;
} hustmq_ha_message_queue_array_t;

ngx_bool_t hustmqha_serialize_message_queue_item(hustmq_ha_message_queue_item_t * obj_val, char * json_val);
ngx_bool_t hustmqha_serialize_message_queue_array(hustmq_ha_message_queue_array_t * obj_val, ngx_pool_t * pool, hustmq_ha_buffer_t * json_val);

typedef struct
{
    const char * worker;
    int time;
} hustmq_worker_t;

typedef struct
{
	hustmq_worker_t * arr;
	size_t size;
} hustmq_worker_array_t;

ngx_bool_t hustmqha_serialize_worker_array(hustmq_worker_array_t * obj_val, ngx_pool_t * pool, hustmq_ha_buffer_t * json_val);

#endif // __hustmq_ha_data_def_20150605185936_h__
