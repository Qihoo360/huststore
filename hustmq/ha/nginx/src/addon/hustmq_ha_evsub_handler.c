#include "hustmq_ha_handler.h"
#include "hustmq_ha_ev_handler.h"
#include "hustmq_ha_handler_filter.h"

typedef struct
{
    hustmq_ha_ev_node_t base;
	unsigned int idx;
} hustmq_ha_evsub_node_t;

static hustmq_ha_ev_dict_t g_evsub_dict;
static ngx_event_t g_evsub_event;
static ngx_int_t g_evsub_timer_count = 0;

static ngx_bool_t __need_active(const hustmq_ha_evsub_node_t * node, const hustmq_ha_message_queue_item_t * queue_item)
{
    if (!queue_item || !node || HUSTMQ_PUSH_QUEUE != queue_item->base.type)
    {
        return false;
    }
    return hustmq_ha_idx_check(node->idx, queue_item->base.idx.start_idx, queue_item->base.idx.consistent_idx);
}

static ngx_str_t __set_idx_value(unsigned int si, unsigned int ci, ngx_pool_t * pool)
{
    enum { BUF_SIZE = 32 };
    ngx_str_t val;
    val.data = ngx_palloc(pool, BUF_SIZE);
    sprintf((char *)val.data, " %d-%d", si, ci);
    val.len = strlen((char *)val.data);
    return val;
}

static void __format_response(unsigned int si, unsigned int ci, ngx_http_request_t * r)
{
    static const ngx_str_t KEY_IDX = ngx_string("Index");
    ngx_http_clear_location(r);
    r->headers_out.status = NGX_HTTP_FORBIDDEN;
    ngx_str_t val = __set_idx_value(si, ci, r->pool);
    if (!ngx_http_add_field_to_headers_out(&KEY_IDX, &val, r))
    {
        r->headers_out.status = NGX_HTTP_NOT_FOUND;
    }
}

hustmq_ha_ev_node_t * __create_ev_node(ngx_http_request_t *r)
{
    int idx = hustmq_ha_get_idx(r);
    if (idx < 0)
    {
        return NULL;
    }

    hustmq_ha_evsub_node_t * node = ngx_palloc(r->pool, sizeof(hustmq_ha_evsub_node_t));
    if (!node)
    {
        return NULL;
    }
    memset(node, 0, sizeof(hustmq_ha_evsub_node_t));
    node->idx = idx;
    return &node->base;
}

static int __get_timeout(ngx_http_hustmq_ha_main_conf_t * conf)
{
    return conf ? conf->subscribe_timeout : 0;
}

ngx_int_t hustmq_ha_evsub_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static const ngx_str_t head = ngx_string("/sub?");
    return hustmq_ha_ev_handler(&head, r, __create_ev_node, __get_timeout, &g_evsub_timer_count, &g_evsub_dict);
}

static void __before_active(const hustmq_ha_message_queue_item_t * queue_item, hustmq_ha_ev_node_t * node)
{
    if (!__need_active((hustmq_ha_evsub_node_t *)node, queue_item))
    {
        __format_response(queue_item->base.idx.start_idx, queue_item->base.idx.consistent_idx, node->r);
    }
}

static void __active_evsub(ngx_event_t * ev)
{
	hustmq_ha_active_ev(&g_evsub_dict, __before_active);
}

void hustmq_ha_init_evsub_handler(ngx_log_t * log)
{
	hustmq_ha_init_ev(log, &g_evsub_dict, &g_evsub_event, __active_evsub);
}

void hustmq_ha_invoke_evsub_handler()
{
	ngx_add_timer(&g_evsub_event, 1);
}
