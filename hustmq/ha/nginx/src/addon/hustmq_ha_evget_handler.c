#include "hustmq_ha_handler.h"
#include "hustmq_ha_ev_handler.h"

static hustmq_ha_ev_dict_t g_evget_dict;
static ngx_event_t g_evget_event;
static ngx_int_t g_evget_timer_count = 0;

static hustmq_ha_ev_node_t * __create_ev_node(ngx_http_request_t *r)
{
    hustmq_ha_ev_node_t * node = ngx_palloc(r->pool, sizeof(hustmq_ha_ev_node_t));
    if (!node)
    {
        return NULL;
    }
    memset(node, 0, sizeof(hustmq_ha_ev_node_t));
    return node;
}

static int __get_timeout(ngx_http_hustmq_ha_main_conf_t * conf)
{
    return conf ? conf->long_polling_timeout : 0;
}

ngx_int_t hustmq_ha_evget_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static const ngx_str_t head = ngx_string("/get?");
    return hustmq_ha_ev_handler(&head, r, __create_ev_node, __get_timeout, &g_evget_timer_count, &g_evget_dict);
}

static void __active_evget(ngx_event_t * ev)
{
	hustmq_ha_active_ev(&g_evget_dict, 0);
}

void hustmq_ha_init_evget_handler(ngx_log_t * log)
{
	hustmq_ha_init_ev(log, &g_evget_dict, &g_evget_event, __active_evget);
}

void hustmq_ha_invoke_evget_handler()
{
	ngx_add_timer(&g_evget_event, 1);
}
