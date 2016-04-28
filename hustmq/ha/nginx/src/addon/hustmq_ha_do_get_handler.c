#include <sys/time.h>
#include "hustmq_ha_request_handler.h"

typedef struct
{
    ngx_queue_t que;
    ngx_event_t ev;
    ngx_http_request_t * r;
    hustmq_ha_do_task_t task;
} hustmq_ha_do_get_ctx_t;

static ngx_str_t __gen_token(uint64_t base, uint64_t delta, ngx_pool_t * pool)
{
    ngx_str_t token;
    token.data = ngx_palloc(pool, 21);
    sprintf((char *)token.data, "%lu", base + delta);
    token.len = strlen((const char *)token.data);
    return token;
}

static ngx_int_t __init_do_task_cache(
    size_t cache_size,
    ngx_pool_t * pool,
    ngx_queue_t * unassigned,
    ngx_queue_t * assigned,
    ngx_queue_t * free)
{
    hustmq_ha_do_get_ctx_t * cached = ngx_pcalloc(pool, sizeof(hustmq_ha_do_get_ctx_t) * cache_size);
    if (!cached)
    {
        return NGX_ERROR;
    }
    memset(cached, 0, sizeof(hustmq_ha_do_get_ctx_t) * cache_size);

    ngx_queue_init(unassigned);
    ngx_queue_init(assigned);
    ngx_queue_init(free);

    struct timeval time;
    struct timezone tz;
    gettimeofday(&time, &tz);

    uint64_t base = time.tv_sec * 1000000 + time.tv_usec;

    uint64_t i = 0;
    for (i = 0; i < cache_size; i++)
    {
        cached[i].task.token = __gen_token(base, i, pool);
        ngx_queue_insert_head(free, &cached[i].que);
    }
    return NGX_OK;
}

static hustmq_ha_do_get_ctx_t * __alloc_task(ngx_queue_t * from, ngx_queue_t * to)
{
    if (ngx_queue_empty(from))
    {
        return NULL;
    }

    ngx_queue_t * q = ngx_queue_head(from);
    ngx_queue_remove(q);
    hustmq_ha_do_get_ctx_t * item = ngx_queue_data(q, hustmq_ha_do_get_ctx_t, que);

    ngx_queue_insert_head(to, q);

    return item;
}

static ngx_int_t __update_task(hustmq_ha_do_get_ctx_t * item, ngx_queue_t * queue)
{
    ngx_queue_remove(&item->que);
    ngx_queue_insert_head(queue, &item->que);
    return NGX_OK;
}

c_dict_t(hustmq_ha_do_get_ctx_t *) g_do_task_dict; // token as key

static ngx_queue_t g_unassigned_task = { 0, 0 };
static ngx_queue_t g_assigned_task = { 0, 0 };
static ngx_queue_t g_free_task = { 0, 0 };

static void __on_do_get_timeout(ngx_event_t * ev)
{
    if (!ev)
    {
        return;
    }
    hustmq_ha_do_get_ctx_t * ctx = ev->data;
    if (!ctx)
    {
        return;
    }
    ngx_http_request_t * r = ctx->r;
    __update_task(ctx, &g_free_task);
    ngx_int_t rc = ngx_http_send_response_imp(NGX_HTTP_REQUEST_TIME_OUT, NULL, r);
    ngx_http_finalize_request(r, rc);
}

ngx_int_t hustmq_ha_init_do_get(ngx_http_hustmq_ha_main_conf_t * mcf)
{
    c_dict_init(&g_do_task_dict);
    return __init_do_task_cache(mcf->do_get_cache_size, mcf->pool, &g_unassigned_task, &g_assigned_task, &g_free_task);
}

hustmq_ha_do_task_t * hustmq_ha_assign_do_task()
{
    hustmq_ha_do_get_ctx_t * ctx = __alloc_task(&g_unassigned_task, &g_assigned_task);
    if (!ctx)
    {
        return NULL;
    }
    if (ctx->ev.timer_set)
    {
        ngx_del_timer(&ctx->ev);
    }
    ngx_http_hustmq_ha_main_conf_t * mcf = hustmq_ha_get_module_main_conf(ctx->r);
    ngx_add_timer(&ctx->ev, mcf->do_get_timeout);
    return &ctx->task;
}

ngx_bool_t hustmq_ha_reply_do_task(const char * token, ngx_buf_t * buf, size_t buf_size)
{
    if (!token || !buf)
    {
        return false;
    }
    hustmq_ha_do_get_ctx_t ** it = c_dict_get(&g_do_task_dict, token);
    if (!it || !*it)
    {
        return false;
    }
    hustmq_ha_do_get_ctx_t * ctx = *it;
    if (ctx->ev.timer_set)
    {
        ngx_del_timer(&ctx->ev);
    }

    ngx_http_request_t * r = ctx->r;
    ngx_int_t rc = hustmq_ha_send_reply(NGX_HTTP_OK, buf, buf_size, r);
    ngx_http_finalize_request(r, rc);

    c_dict_remove(&g_do_task_dict, (const char *)ctx->task.token.data);
    __update_task(ctx, &g_free_task);

    return true;
}

static ngx_bool_t __do_get(ngx_http_request_t * r)
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
    if (buf_size < 1 || buf_size > (size_t) mcf->max_do_task_body_size)
    {
        return false;
    }
    if (!buf || buf_size < 1)
    {
        return false;
    }

    ngx_str_t queue = hustmq_ha_get_queue(r);
    if (!queue.data)
    {
        return false;
    }

    hustmq_ha_do_get_ctx_t * ctx = __alloc_task(&g_free_task, &g_unassigned_task);
    if (!ctx)
    {
        return false;
    }
    ctx->task.queue = queue;
    ctx->task.buf = buf;
    ctx->task.buf_size = buf_size;
    ctx->r = r;
    c_dict_set(&g_do_task_dict, (const char *)ctx->task.token.data, ctx);

    if (hustmq_ha_dispatch_do_task(&ctx->task))
    {
        __update_task(ctx, &g_assigned_task);
    }

    ctx->ev.handler = __on_do_get_timeout;
    ctx->ev.log = mcf->log;
    ctx->ev.data = ctx;
    ngx_add_timer(&ctx->ev, mcf->do_get_timeout);

    return true;
}

static void __post_body_handler(ngx_http_request_t * r)
{
    if (!__do_get(r))
    {
        ngx_int_t rc = ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
        ngx_http_finalize_request(r, rc);
    }
}

ngx_int_t hustmq_ha_do_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    if (!(r->method & NGX_HTTP_POST))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }
    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
    return (rc >= NGX_HTTP_SPECIAL_RESPONSE) ? rc : NGX_DONE;
}

ngx_int_t hustmq_ha_do_get_status_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    size_t unassigned_tasks = hustmq_ha_get_queue_size(&g_unassigned_task);
    size_t assigned_tasks = hustmq_ha_get_queue_size(&g_assigned_task);
    size_t free_tasks = hustmq_ha_get_queue_size(&g_free_task);
    size_t total = unassigned_tasks + assigned_tasks + free_tasks;

    char buf[512] = {0};
    sprintf(buf, "{\"unassigned_tasks\":%lu,\"assigned_tasks\":%lu,\"free_tasks\":%lu,\"total\":%lu}",
        unassigned_tasks, assigned_tasks, free_tasks, total);
    ngx_str_t response = { strlen(buf), (u_char *)buf };
    return ngx_http_send_response_imp(NGX_HTTP_OK, &response, r);
}
