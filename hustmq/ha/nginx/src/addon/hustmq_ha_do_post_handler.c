#include "hustmq_ha_request_handler.h"

typedef struct
{
    ngx_queue_t que;
    ngx_event_t ev;
    ngx_http_request_t * r;
} hustmq_ha_do_post_ctx_t;

static ngx_int_t __init_executer_cache(
    size_t cache_size,
    ngx_pool_t * pool,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    hustmq_ha_do_post_ctx_t * cached = ngx_pcalloc(pool, sizeof(hustmq_ha_do_post_ctx_t) * cache_size);
    if (!cached)
    {
        return NGX_ERROR;
    }
    memset(cached, 0, sizeof(hustmq_ha_do_post_ctx_t) * cache_size);

    ngx_queue_init(cache);
    ngx_queue_init(free);

    uint64_t i = 0;
    for (i = 0; i < cache_size; i++)
    {
        ngx_queue_insert_head(free, &cached[i].que);
    }
    return NGX_OK;
}

static hustmq_ha_do_post_ctx_t * __alloc_executer(ngx_queue_t * from, ngx_queue_t * to)
{
    if (ngx_queue_empty(from))
    {
        return NULL;
    }

    ngx_queue_t * q = ngx_queue_head(from);
    ngx_queue_remove(q);
    hustmq_ha_do_post_ctx_t * item = ngx_queue_data(q, hustmq_ha_do_post_ctx_t, que);

    ngx_queue_insert_head(to, q);

    return item;
}

static ngx_int_t __free_executer(hustmq_ha_do_post_ctx_t * item, ngx_queue_t * free)
{
    ngx_queue_remove(&item->que);
    ngx_queue_insert_head(free, &item->que);
    return NGX_OK;
}

static ngx_queue_t g_cache_worker = { 0, 0 };
static ngx_queue_t g_free_worker = { 0, 0 };

ngx_int_t hustmq_ha_init_do_post(ngx_http_hustmq_ha_main_conf_t * mcf)
{
    return __init_executer_cache(mcf->do_post_cache_size, mcf->pool, &g_cache_worker, &g_free_worker);
}

static ngx_bool_t __send_reply(hustmq_ha_do_task_t * task, ngx_http_request_t * r)
{
    if (!task || !r)
    {
        return false;
    }
    static ngx_str_t queue = ngx_string("Queue");
    if (!ngx_http_add_field_to_headers_out(&queue, &task->queue, r))
    {
        return false;
    }

    static ngx_str_t token = ngx_string("Token");
    if (!ngx_http_add_field_to_headers_out(&token, &task->token, r))
    {
        return false;
    }

    ngx_int_t rc = hustmq_ha_send_reply(NGX_HTTP_OK, task->buf, task->buf_size, r);
    ngx_http_finalize_request(r, rc);
    return true;
}

ngx_bool_t hustmq_ha_dispatch_do_task(hustmq_ha_do_task_t * task)
{
    if (!task || ngx_queue_empty(&g_cache_worker))
    {
        return false;
    }
    ngx_queue_t * q = ngx_queue_head(&g_cache_worker);
    hustmq_ha_do_post_ctx_t * item = ngx_queue_data(q, hustmq_ha_do_post_ctx_t, que);
    ngx_bool_t rc = __send_reply(task, item->r);
    if (rc)
    {
        if (item->ev.timer_set)
        {
            ngx_del_timer(&item->ev);
        }
        __free_executer(item, &g_free_worker);
    }
    return rc;
}

static void __on_do_post_timeout(ngx_event_t * ev)
{
    if (!ev)
    {
        return;
    }
    hustmq_ha_do_post_ctx_t * ctx = ev->data;
    if (!ctx)
    {
        return;
    }
    ngx_http_request_t * r = ctx->r;
    __free_executer(ctx, &g_free_worker);
    ngx_int_t rc = ngx_http_send_response_imp(NGX_HTTP_REQUEST_TIME_OUT, NULL, r);
    ngx_http_finalize_request(r, rc);
}

static ngx_bool_t __claim_task(ngx_http_request_t * r)
{
    hustmq_ha_do_task_t * task = hustmq_ha_assign_do_task();
    if (!task)
    {
        ngx_http_hustmq_ha_main_conf_t * mcf = hustmq_ha_get_module_main_conf(r);
        if (!mcf)
        {
            return false;
        }
        hustmq_ha_do_post_ctx_t * ctx = __alloc_executer(&g_free_worker, &g_cache_worker);
        if (!ctx)
        {
            return false;
        }
        ctx->r = r;

        ctx->ev.handler = __on_do_post_timeout;
        ctx->ev.log = mcf->log;
        ctx->ev.data = ctx;
        ngx_add_timer(&ctx->ev, mcf->do_post_timeout);

        return true;
    }
    return __send_reply(task, r);
}

static ngx_bool_t __reply_task(const char * token, ngx_http_request_t * r)
{
    ngx_http_hustmq_ha_main_conf_t * mcf = hustmq_ha_get_module_main_conf(r);
    if (!mcf)
    {
        return false;
    }
    ngx_buf_t * buf = ngx_http_get_request_body(r);
    if (!buf)
    {
        return false;
    }
    size_t buf_size = ngx_http_get_buf_size(buf);
    if (buf_size < 1)
    {
        return false;
    }
    if (!buf || buf_size < 1 || buf_size > (size_t)mcf->max_do_task_body_size)
    {
        return false;
    }

    ngx_bool_t ok = hustmq_ha_reply_do_task(token, buf, buf_size); // call do get

    ngx_int_t rc = ngx_http_send_response_imp(ok ? NGX_HTTP_OK : NGX_HTTP_NOT_FOUND, NULL, r);
    ngx_http_finalize_request(r, rc);
    return true;
}

static void __post_body_handler(ngx_http_request_t * r)
{
    char * val = ngx_http_get_param_val(&r->args, "token", r->pool);
    ngx_bool_t rc = val ? __reply_task(val, r) : __claim_task(r);
    if (!rc)
    {
        ngx_int_t rc = ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
        ngx_http_finalize_request(r, rc);
    }
}

ngx_int_t hustmq_ha_do_post_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    if (!(r->method & NGX_HTTP_POST))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }
    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
    return (rc >= NGX_HTTP_SPECIAL_RESPONSE) ? rc : NGX_DONE;
}

ngx_int_t hustmq_ha_do_post_status_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    size_t cache_workers = hustmq_ha_get_queue_size(&g_cache_worker);
    size_t free_workers = hustmq_ha_get_queue_size(&g_free_worker);
    size_t total = cache_workers + free_workers;
    char buf[512] = {0};
    sprintf(buf, "{\"cache_workers\":%lu,\"free_workers\":%lu,\"total\":%lu}",
        cache_workers, free_workers, total);
    ngx_str_t response = { strlen(buf), (u_char *)buf };
    return ngx_http_send_response_imp(NGX_HTTP_OK, &response, r);
}
