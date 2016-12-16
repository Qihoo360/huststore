#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_data_def.h"
#include "hustmq_ha_peer_def.h"
#include "hustmq_ha_handler_filter.h"
#include "hustmq_ha_request_handler.h"

ngx_int_t hustmq_ha_version_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static ngx_str_t version = ngx_string("hustmqha 1.6\n");
    r->headers_out.status = NGX_HTTP_OK;
    return ngx_http_send_response_imp(r->headers_out.status, &version, r);
}

static ngx_bool_t __check_lock_request(ngx_http_request_t *r, hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
    char * val = ngx_http_get_param_val(&r->args, "on", r->pool);
    if (!val)
    {
        return false;
    }
    size_t size = strlen(val);
    if (1 != size)
    {
        return false;
    }
    return '0' == val[0] || '1' == val[0];
}

ngx_int_t hustmq_ha_lock_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustmq_ha_handler_base(backend_uri, r, __check_lock_request, hustmq_ha_post_subrequest_handler);
}

static ngx_bool_t __check_purge_request(ngx_http_request_t *r, hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
    char * val = ngx_http_get_param_val(&r->args, "priori", r->pool);
    if (!val)
    {
        return false;
    }
    size_t size = strlen(val);
    if (1 != size)
    {
        return false;
    }
    return '0' == val[0] || '1' == val[0] || '2' == val[0];
}

ngx_int_t hustmq_ha_purge_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustmq_ha_handler_base(backend_uri, r, __check_purge_request, hustmq_ha_post_subrequest_handler);
}

static ngx_bool_t __is_number(const char * param)
{
    if (!param)
    {
        return false;
    }

    size_t len = strlen(param);
    size_t i = 0;
    for (i = 0; i < len; ++i)
    {
        if (param[i] < '0' || param[i] > '9')
        {
            return false;
        }
    }
    return true;
}

static ngx_bool_t __check_max_request(ngx_http_request_t *r, hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
    if (queue_dict && queue_dict->dict.ref && !hustmq_ha_max_queue_item_check(queue_dict, queue))
    {
        return false;
    }
    char * val = (ngx_http_get_param_val(&r->args, "num", r->pool));
    return __is_number(val);
}

ngx_int_t hustmq_ha_max_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustmq_ha_handler_base(backend_uri, r, __check_max_request, hustmq_ha_post_subrequest_handler);
}

static ngx_bool_t __check_timeout_request(ngx_http_request_t *r, hustmq_ha_queue_dict_t * queue_dict, ngx_str_t * queue)
{
    char * val = ngx_http_get_param_val(&r->args, "minute", r->pool);
    if (!val)
    {
        return false;
    }
    int timeout = atoi(val);
    return timeout > 0 && timeout < 256;
}

ngx_int_t hustmq_ha_timeout_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    return hustmq_ha_handler_base(backend_uri, r, __check_timeout_request, hustmq_ha_post_subrequest_handler);
}

ngx_int_t hustmq_ha_ack_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_subrequest_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        static ngx_str_t PEER = ngx_string("peer");

        char * val = ngx_http_get_param_val(&r->args, (const char *)PEER.data, r->pool);
        if (!val)
        {
            return NGX_ERROR;
        }
        ngx_str_t ack_peer = { strlen(val), (u_char *)val };
        ngx_http_upstream_rr_peer_t * peer = hustmq_ha_decode_ack_peer(&ack_peer, r->pool);
        if (!peer)
        {
            return NGX_ERROR;
        }

        ngx_http_subrequest_ctx_t * ctx = ngx_palloc(r->pool, sizeof(ngx_http_subrequest_ctx_t));
        if (!ctx)
        {
            return NGX_ERROR;
        }
        memset(ctx, 0, sizeof(ngx_http_subrequest_ctx_t));
        ngx_http_set_addon_module_ctx(r, ctx);

        ctx->args = ngx_http_remove_param(&r->args, &PEER, r->pool);

        return ngx_http_gen_subrequest(backend_uri, r, peer,
            ctx, ngx_http_post_subrequest_handler);
    }
    return ngx_http_send_response_imp(r->headers_out.status, NULL, r);
}

ngx_int_t hustmq_ha_stat_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_str_t queue = hustmq_ha_get_queue(r);
    if (!queue.data)
    {
        return NGX_ERROR;
    }

    hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();

    do
    {
        if (!queue_dict)
        {
            break;
        }

        static hustmq_ha_message_queue_item_t queue_item;
        if (!hustmq_ha_merge_queue_item(queue_dict, &queue, &queue_item))
        {
            break;
        }

        static char response[HUSTMQ_HA_QUEUE_ITEM_SIZE];
        ngx_bool_t ret = hustmqha_serialize_message_queue_item(&queue_item, response);
        if (!ret)
        {
            break;
        }

        ngx_str_t tmp;
        tmp.data = (u_char *) response;
        tmp.len = strlen(response);

        return ngx_http_send_response_imp(NGX_HTTP_OK, &tmp, r);

    } while (0);

    return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
}
