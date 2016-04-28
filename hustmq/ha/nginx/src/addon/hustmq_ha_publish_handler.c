#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_handler_filter.h"
#include "hustmq_ha_peer_def.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_subrequest_peer_t * subrequest_peer;
    size_t count;
    ngx_str_t args;
    hustmq_ha_queue_value_t * queue_val;
    int * ref_count;
    ngx_bool_t timeout;
    ngx_event_t ev;
} hustmq_ha_publish_ctx_t;

typedef c_dict_int_t hustmq_ha_pub_ref_dict_t;
static hustmq_ha_pub_ref_dict_t g_pub_ref_dict;

void hustmq_ha_init_pub_ref_dict()
{
    c_dict_init(&g_pub_ref_dict);
}

static void __inc_ref_count(int * ref_count)
{
    if (ref_count)
    {
        ++*ref_count;
    }
}

static void __dec_ref_count(int * ref_count)
{
    if (ref_count && *ref_count > 0)
    {
        --*ref_count;
    }
}

static void __reset_ref_count(int * ref_count)
{
    if (ref_count)
    {
        *ref_count = 0;
    }
}

static int * __get_ref_count(ngx_str_t * queue, ngx_pool_t * pool)
{
    if (!queue)
    {
        return NULL;
    }
    int * val = c_dict_get(&g_pub_ref_dict, (const char *)queue->data);
    if (!val)
    {
	    ngx_str_t key = ngx_http_make_str(queue, pool);
        c_dict_set(&g_pub_ref_dict, (const char *)key.data, 0);
        val = c_dict_get(&g_pub_ref_dict, (const char *)queue->data);
    }
    return val;
}

static ngx_str_t __cat_idx(unsigned int idx, ngx_pool_t * pool, ngx_str_t str)
{
    enum { MAX_NUM_LEN = 32 };
    static ngx_str_t IDX_KEY = ngx_string("&idx=");

    size_t offset = str.len + IDX_KEY.len;
    size_t size = offset + MAX_NUM_LEN;

    ngx_str_t args;
    args.data = ngx_palloc(pool, size);
    memset(args.data, 0, size);
    memcpy(args.data, str.data, str.len);
    memcpy(args.data + str.len, IDX_KEY.data, IDX_KEY.len);
    sprintf((char *)(args.data + offset), "%d", idx);
    args.len = strlen((char *)args.data);
    return args;
}

static ngx_str_t __get_args(unsigned int idx, ngx_http_request_t * r)
{
    idx = (idx + 1) % CYCLE_QUEUE_ITEM_NUM;
    if (hustmq_ha_get_idx(r) < 0)
    {
        return __cat_idx(idx, r->pool, r->args);
    }
    static ngx_str_t IDX = ngx_string("idx");
    ngx_str_t args = ngx_http_remove_param(&r->args, &IDX, r->pool);
    return __cat_idx(idx, r->pool, args);
}

static void __update_args(ngx_http_request_t * r, hustmq_ha_publish_ctx_t * ctx)
{
    if (ctx->queue_val && hustmq_ha_inconsistent(ctx->subrequest_peer))
    {
        ctx->base.args = ctx->args;
    }
    else
    {
        ctx->base.args.data = NULL;
        ctx->base.args.len = 0;
    }
}

static void __publish_timeout_handler(ngx_event_t * ev)
{
    if (!ev)
    {
        return;
    }
    hustmq_ha_publish_ctx_t * ctx = ev->data;
    if (ctx)
    {
        ctx->timeout = true;

        // important! maybe the subrequest never return
        // so you need to reset ref_count here
        __reset_ref_count(ctx->ref_count);
    }
}

static void __post_body_handler(ngx_http_request_t * r)
{
    hustmq_ha_publish_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    __update_args(r, ctx);
    --r->main->count;
    ngx_int_t rc = ngx_http_gen_subrequest(ctx->base.backend_uri, r, ctx->subrequest_peer->peer,
        &ctx->base, ngx_http_post_subrequest_handler);
    if (NGX_DONE == rc)
    {
        ngx_http_hustmq_ha_main_conf_t * conf = hustmq_ha_get_module_main_conf(r);
        ngx_add_timer(&ctx->ev, conf->publish_timeout);
        __inc_ref_count(ctx->ref_count);
    }
}

ngx_int_t __first_publish_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_str_t queue = hustmq_ha_get_queue(r);
    if (!queue.data)
    {
        return NGX_ERROR;
    }

    ngx_http_hustmq_ha_main_conf_t * conf = hustmq_ha_get_module_main_conf(r);
    if (!conf)
    {
        return NGX_ERROR;
    }

    int * ref_count = __get_ref_count(&queue, conf->pool);
    if (!ref_count || *ref_count > 0)
    {
        return NGX_ERROR;
    }

    hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();
    if (!queue_dict)
    {
        return NGX_ERROR;
    }

    hustmq_ha_queue_ctx_t * queue_ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_queue_ctx_t));
    if (!queue_ctx)
    {
        return NGX_ERROR;
    }
    memset(queue_ctx, 0, sizeof(hustmq_ha_queue_ctx_t));
    if (queue_dict->dict.ref && !hustmq_ha_publish_queue_item_check(queue_dict, &queue, queue_ctx))
    {
        return NGX_ERROR;
    }

    ngx_http_subrequest_peer_t * peer = hustmq_ha_build_pub_peer_list(queue_ctx, r->pool);
    if (!peer)
    {
        return NGX_ERROR;
    }

    peer = ngx_http_get_first_peer(peer);
    if (!peer)
    {
        return NGX_ERROR;
    }

    hustmq_ha_publish_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_publish_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(hustmq_ha_publish_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);

    ctx->base.backend_uri = backend_uri;

    ctx->subrequest_peer = peer;

    ctx->count = 0;
    ctx->ref_count = ref_count;
    ctx->timeout = false;
    ctx->queue_val = queue_ctx->queue_val;
    ctx->args = __get_args(queue_ctx->item.idx.consistent_idx, r);

    ctx->ev.handler = __publish_timeout_handler;
    ctx->ev.log = conf->log;
    ctx->ev.data = ctx;

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

ngx_int_t hustmq_ha_publish_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustmq_ha_publish_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __first_publish_handler(backend_uri, r);
    }
    if (!ctx->timeout)
    {
        if (NGX_HTTP_OK == r->headers_out.status)
        {
            ++ctx->count;
        }
        ctx->subrequest_peer = ngx_http_get_next_peer(ctx->subrequest_peer);
        if (ctx->subrequest_peer)
        {
            __update_args(r, ctx);
            ngx_int_t rc = ngx_http_run_subrequest(r, &ctx->base, ctx->subrequest_peer->peer);
            if (NGX_DONE == rc)
            {
                return NGX_DONE;
            }
        }
        // you can only del timer when not expired
        ngx_del_timer(&ctx->ev);
    }

    __dec_ref_count(ctx->ref_count);

    size_t backends = ngx_http_get_backend_count();
    if (ctx->count > 0 && ctx->count < backends)
    {
        static ngx_str_t KEY = ngx_string("count");
        char * buf = ngx_palloc(r->pool, 32);
        sprintf(buf, "%d", (int)ctx->count);
        ngx_str_t val = { strlen(buf), (u_char *)buf };
        ngx_http_add_field_to_headers_out(&KEY, &val, r);
    }
    return ngx_http_send_response_imp(ctx->count > 0 ? NGX_HTTP_OK : NGX_HTTP_NOT_FOUND, &ctx->base.response, r);
}
