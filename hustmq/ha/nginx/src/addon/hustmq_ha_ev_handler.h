#ifndef __hustmq_ha_ev_handler_20151016104222_h__
#define __hustmq_ha_ev_handler_20151016104222_h__

#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"

typedef struct
{
    ngx_http_request_t * r;
    ngx_int_t * ev_timer_count;
    ngx_event_t ev;
    ngx_queue_t elem;
} hustmq_ha_ev_node_t;

typedef struct
{
    c_dict_t(ngx_queue_t *) dict; // queue as key
    size_t size;
} hustmq_ha_ev_dict_t;

typedef int (*hustmq_ha_get_timeout_t)(ngx_http_hustmq_ha_main_conf_t * conf);
typedef hustmq_ha_ev_node_t * (*hustmq_ha_create_ev_node_t)(ngx_http_request_t *r);
typedef void (*hustmq_ha_before_active_t)(const hustmq_ha_message_queue_item_t * queue_item, hustmq_ha_ev_node_t * node);

ngx_int_t hustmq_ha_ev_handler(
        const ngx_str_t * head,
        ngx_http_request_t *r,
        hustmq_ha_create_ev_node_t create,
        hustmq_ha_get_timeout_t get_timeout,
        ngx_int_t * ev_timer_count,
        hustmq_ha_ev_dict_t * ev_dict);

void hustmq_ha_init_ev(
        ngx_log_t * log,
        hustmq_ha_ev_dict_t * ev_dict,
        ngx_event_t * ev_event,
        ngx_event_handler_pt handler);

hustmq_ha_ev_node_t * hustmq_ha_pop_ev_node(ngx_queue_t * que);
void hustmq_ha_active_ev(hustmq_ha_ev_dict_t * ev_dict, hustmq_ha_before_active_t before_active);

#endif // __hustmq_ha_ev_handler_20151016104222_h__
