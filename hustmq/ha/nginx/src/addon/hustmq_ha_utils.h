#ifndef __hustmq_ha_utils_20150601202703_h__
#define __hustmq_ha_utils_20150601202703_h__

#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_addon_def.h>
#include <ngx_http_utils_module.h>
#include <ngx_http_fetch.h>
#include "message_queue_def.h"
#include "worker_def.h"

#undef HUSTMQ_HA_QUEUE_SIZE
#define HUSTMQ_HA_QUEUE_SIZE      64

#undef HUSTMQ_HA_READY_SIZE
#define HUSTMQ_HA_READY_SIZE      3

#undef HUSTMQ_HA_QUEUE_ITEM_SIZE
#define HUSTMQ_HA_QUEUE_ITEM_SIZE 1024

#undef HUSTMQ_HA_WORKER_SIZE
#define HUSTMQ_HA_WORKER_SIZE     32

#undef HUSTMQ_HA_WORKER_ITEMS
#define HUSTMQ_HA_WORKER_ITEMS    1024

#define CYCLE_QUEUE_ITEM_NUM      11000000
#define MAX_QUEUE_ITEM_NUM        5000000

typedef struct
{
    ngx_pool_t * pool;
    ngx_log_t * log;
    ngx_str_t prefix;
    ssize_t max_queue_size;
    ngx_bool_t queue_hash;
    ngx_int_t long_polling_timeout;
    ngx_int_t subscribe_timeout;
    ngx_int_t publish_timeout;
    ssize_t max_timer_count;
    ngx_bool_t status_cache;
    ssize_t fetch_req_pool_size;
    ngx_int_t keepalive_cache_size;
    ngx_int_t connection_cache_size;
    ngx_str_t autost_uri;
    ngx_str_t username;
    ngx_str_t password;
    ngx_int_t fetch_connect_timeout;
    ngx_int_t fetch_send_timeout;
    ngx_int_t fetch_read_timeout;
    ngx_int_t fetch_timeout;
    ssize_t fetch_buffer_size;
    ngx_int_t autost_interval;
    ngx_int_t do_post_cache_size;
    ngx_int_t do_get_cache_size;
    ssize_t max_do_task_body_size;
    ngx_int_t do_post_timeout;
    ngx_int_t do_get_timeout;
} ngx_http_hustmq_ha_main_conf_t;

typedef enum
{
    HUSTMQ_NORMAL_QUEUE = 0,
    HUSTMQ_PUSH_QUEUE = 1
} hustmq_queue_type_t;

void * hustmq_ha_get_module_main_conf(ngx_http_request_t * r);

ngx_http_upstream_rr_peers_t * ngx_http_get_backends();
size_t ngx_http_get_backend_count();

ngx_uint_t hustmq_ha_get_max_queue_size();
ngx_bool_t hustmq_ha_check_queue(const char * qname, const int qname_len);
int hustmq_ha_get_ready_sum(const int * arr, size_t size);
unsigned int hustmq_ha_get_idx_cycle(unsigned int start, unsigned int end);

void hustmq_ha_copy_from_integer_array(const IntegerArray * src, int * des, size_t max_size);
void hustmq_ha_copy_from_json_str(const json_str_t * src, char * des, size_t max_size);

typedef ngx_bool_t (*hustmq_ha_is_invalid_t)(c_dict_base_t * dict, const char * key);
void hustmq_ha_clean_invalid_items(c_dict_base_t * dict, hustmq_ha_is_invalid_t is_invalid, size_t * dict_size);
size_t hustmq_ha_get_queue_size(ngx_queue_t * queue);

#endif // __hustmq_ha_utils_20150601202703_h__
