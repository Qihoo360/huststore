#include "hustmq_ha_ev_handler.h"

static ngx_queue_t * hustmq_ha_ev_dict_get(hustmq_ha_ev_dict_t * dict, const char * key)
{
    if (!dict)
    {
        return 0;
    }
    ngx_queue_t ** val = c_dict_get(&dict->dict, key);
    return val ? *val : 0;
}

static void __ev_timeout_handler(ngx_event_t * ev)
{
    if (!ev)
    {
        return;
    }
    hustmq_ha_ev_node_t * node = ev->data;
    if (!node)
    {
        return;
    }
    ngx_http_request_t * r = node->r;
    ngx_int_t * ev_timer_count = node->ev_timer_count;
    if (!r || !ev_timer_count)
    {
        return;
    }

    --*ev_timer_count;

    ngx_queue_remove(&node->elem);
    ngx_http_clear_location(r);
    ngx_int_t rc = ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    ngx_http_finalize_request(r, rc);
}

static ngx_bool_t __add_ev_node(hustmq_ha_ev_dict_t * ev_dict, ngx_pool_t * pool, ngx_str_t * queue, hustmq_ha_ev_node_t * node)
{
    if (ev_dict->size > hustmq_ha_get_max_queue_size() - 1)
    {
        return false;
    }
    ngx_queue_t * it_que = hustmq_ha_ev_dict_get(ev_dict, (const char *)queue->data);
    if (!it_que)
    {
        ngx_queue_t * que_val = ngx_palloc(pool, sizeof(ngx_queue_t));
        if (!que_val)
        {
            return false;
        }
        memset(que_val, 0, sizeof(ngx_queue_t));
        ngx_queue_init(que_val);
        ngx_queue_insert_tail(que_val, &node->elem);
        ngx_str_t key = ngx_http_make_str(queue, pool);
        c_dict_set(&ev_dict->dict, (const char *)key.data, que_val);
        ++ev_dict->size;
    }
    else
    {
        ngx_queue_insert_tail(it_que, &node->elem);
    }
    return true;
}

ngx_int_t hustmq_ha_ev_handler(
        const ngx_str_t * head,
        ngx_http_request_t *r,
        hustmq_ha_create_ev_node_t create,
        hustmq_ha_get_timeout_t get_timeout,
        ngx_int_t * ev_timer_count,
        hustmq_ha_ev_dict_t * ev_dict)
{
    ngx_http_hustmq_ha_main_conf_t * conf = hustmq_ha_get_module_main_conf(r);
    if (!conf || !ev_timer_count || *ev_timer_count >= conf->max_timer_count)
    {
        return NGX_ERROR;
    }

    ngx_str_t queue = hustmq_ha_get_queue(r);
    if (!queue.data)
    {
        return NGX_ERROR;
    }

    static const size_t LOCATION_SIZE = 256;
    if (!ngx_http_make_redirect(LOCATION_SIZE, head, r))
    {
        return NGX_ERROR;
    }
	
    hustmq_ha_ev_node_t * node = create(r);
    if (!node)
    {
        return NGX_ERROR;
    }

    node->r = r;
    node->ev_timer_count = ev_timer_count;
    node->elem.next = NULL;
    node->elem.prev = NULL;

    ngx_memzero(&node->ev, sizeof(ngx_event_t));
    node->ev.handler = __ev_timeout_handler;
    node->ev.log = conf->log;
    node->ev.data = node;

    if (!__add_ev_node(ev_dict, conf->pool, &queue, node))
    {
        return NGX_ERROR;
    }

    ngx_add_timer(&node->ev, get_timeout(conf));
    ++*ev_timer_count;

    r->main->count++;
    r->write_event_handler = ngx_http_request_empty_handler;
    return NGX_DONE;
}

void hustmq_ha_init_ev(
        ngx_log_t * log,
        hustmq_ha_ev_dict_t * ev_dict,
        ngx_event_t * ev_event,
        ngx_event_handler_pt handler)
{
    c_dict_init(&ev_dict->dict);
    ev_dict->size = 0;
    ngx_memzero(ev_event, sizeof(ngx_event_t));
    ev_event->handler = handler;
    ev_event->log = log;
}

hustmq_ha_ev_node_t * hustmq_ha_pop_ev_node(ngx_queue_t * que)
{
    if (ngx_queue_empty(que))
    {
        return NULL;
    }

    ngx_queue_t * item = ngx_queue_head(que);
    ngx_queue_remove(item);

    hustmq_ha_ev_node_t * node = ngx_queue_data(item, hustmq_ha_ev_node_t, elem);
    if (!node)
    {
        return NULL;
    }
    ngx_del_timer(&node->ev);
    --*(node->ev_timer_count);
    return node;
}

static void __active_ev_queue(int ready, const hustmq_ha_message_queue_item_t * queue_item, ngx_queue_t * que, hustmq_ha_before_active_t before_active)
{
    while (ready > 0)
    {
        hustmq_ha_ev_node_t * node = hustmq_ha_pop_ev_node(que);
        if (!node)
        {
            break;
        }
        if (before_active)
        {
            before_active(queue_item, node);
        }
        ngx_http_finalize_request(node->r, ngx_http_send_response_imp(node->r->headers_out.status, NULL, node->r));
        --ready;
    }
}

static ngx_bool_t __is_dict_item_invalid(c_dict_base_t * dict, const char * key)
{
    ngx_queue_t * it_que = hustmq_ha_ev_dict_get((hustmq_ha_ev_dict_t *)dict, key);
    return (!it_que || ngx_queue_empty(it_que));
}

void hustmq_ha_active_ev(hustmq_ha_ev_dict_t * ev_dict, hustmq_ha_before_active_t before_active)
{
    hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();
    if (!queue_dict)
    {
        return;
    }

    if (ev_dict->size > hustmq_ha_get_max_queue_size() - 1)
    {
        hustmq_ha_clean_invalid_items(&ev_dict->dict.base, __is_dict_item_invalid, &ev_dict->size);
    }

    const char * queue;
    c_dict_iter_t iter = c_dict_iter(&ev_dict->dict);
    while ((queue = c_dict_next(&ev_dict->dict, &iter)))
    {
        ngx_queue_t * it_que = hustmq_ha_ev_dict_get(ev_dict, queue);
        if (!it_que)
        {
            continue;
        }
        hustmq_ha_message_queue_item_t queue_item;
        ngx_str_t key = { strlen(queue), (u_char *)queue };
        if (!hustmq_ha_merge_queue_item(queue_dict, &key, &queue_item))
        {
            continue;
        }
        int ready = hustmq_ha_get_ready_sum(queue_item.base.ready, HUSTMQ_HA_READY_SIZE);
        if (ready < 1)
        {
            continue;
        }
        __active_ev_queue(ready, &queue_item, it_que, before_active);
    }
}
