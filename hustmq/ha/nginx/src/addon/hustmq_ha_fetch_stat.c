#include "hustmq_ha_fetch_stat.h"

typedef struct
{
    ngx_queue_t queue;
    ngx_pool_t * pool;
    ngx_event_t ev;

    size_t requests;
    size_t finished;
    backend_stat_array_t backend_stats;
} autost_ctx_cache_t;

static ngx_int_t __init_cache(
    size_t cache_size,
    ngx_pool_t * pool,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    autost_ctx_cache_t * cached = ngx_pcalloc(pool, sizeof(autost_ctx_cache_t) * cache_size);
    if (!cached)
    {
        return NGX_ERROR;
    }
    memset(cached, 0, sizeof(autost_ctx_cache_t) * cache_size);

    ngx_queue_init(cache);
    ngx_queue_init(free);

    size_t i = 0;
    for (i = 0; i < cache_size; i++)
    {
        ngx_queue_insert_head(free, &cached[i].queue);
    }
    return NGX_OK;
}

static autost_ctx_cache_t * __get_free_item(ngx_queue_t * cache, ngx_queue_t * free)
{
    if (ngx_queue_empty(free))
    {
        return NULL;
    }

    ngx_queue_t * q = ngx_queue_head(free);
    ngx_queue_remove(q);
    autost_ctx_cache_t * item = ngx_queue_data(q, autost_ctx_cache_t, queue);

    ngx_queue_insert_head(cache, q);

    return item;
}

static ngx_int_t __reuse_item(autost_ctx_cache_t * item, ngx_queue_t * free)
{
    ngx_queue_remove(&item->queue);
    ngx_queue_insert_head(free, &item->queue);
    return NGX_OK;
}

static ngx_queue_t g_cache_ctx = { 0, 0 };
static ngx_queue_t g_free_ctx = { 0, 0 };
static ngx_event_t g_ev_event;
static ngx_http_hustmq_ha_main_conf_t * g_mcf = NULL;
static hustmq_ha_merge_backend_stats_t g_merge = NULL;
static hustmq_ha_set_backend_stat_item_t g_set = NULL;

ngx_int_t hustmq_ha_init_fetch_cache(
    ngx_http_hustmq_ha_main_conf_t * mcf,
    hustmq_ha_set_backend_stat_item_t set,
    hustmq_ha_merge_backend_stats_t merge)
{
    if (!mcf || !set || !merge)
    {
        return NGX_ERROR;
    }
    g_mcf = mcf;
    g_merge = merge;
    g_set = set;
    return __init_cache(
        mcf->fetch_req_pool_size * ngx_http_get_backend_count(),
        mcf->pool,
        &g_cache_ctx,
        &g_free_ctx);
}

static void __finalize_ctx(autost_ctx_cache_t * ctx, ngx_http_hustmq_ha_main_conf_t * conf)
{
    if (!ctx->pool)
    {
        return;
    }
    g_merge(conf, &ctx->backend_stats);
    ngx_destroy_pool(ctx->pool);
    ctx->pool = NULL;
    __reuse_item(ctx, &g_free_ctx);
}

static ngx_int_t __post_upstream(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    autost_ctx_cache_t * ctx = data;
    ++ctx->finished;

    if (!ctx->ev.timer_set)
    {
        ngx_http_fetch_log("ngx_http_fetch::already timeout");
        return NGX_OK;
    }

    if (NGX_OK == rc && g_set(r, ctx->backend_stats.arr + ctx->backend_stats.size))
    {
        ++ctx->backend_stats.size;
    }
    if (ctx->finished >= ctx->requests)
    {
        ngx_del_timer(&ctx->ev);
        __finalize_ctx(ctx, g_mcf);
    }
    return NGX_OK;
}

static void __on_fetch_timeout(ngx_event_t * ev)
{
    ngx_http_fetch_log("ngx_http_fetch::__on_fetch_timeout");
    autost_ctx_cache_t * ctx = ev->data;
    __finalize_ctx(ctx, g_mcf);
}

autost_ctx_cache_t * __get_autost_ctx(ngx_http_hustmq_ha_main_conf_t * mcf)
{
    autost_ctx_cache_t * ctx = __get_free_item(&g_cache_ctx, &g_free_ctx);
    if (!ctx)
    {
        return NULL;
    }

    do
    {
        ctx->pool = ngx_create_pool(mcf->fetch_req_pool_size, mcf->log);
        if (!ctx->pool)
        {
            break;
        }
        ctx->requests = ngx_http_get_backend_count();
        ctx->finished = 0;
        ctx->backend_stats.max_size = ctx->requests;
        ctx->backend_stats.size = 0;
        ctx->backend_stats.arr = ngx_palloc(ctx->pool, ctx->backend_stats.max_size * sizeof(backend_stat_item_t));
        if (!ctx->backend_stats.arr)
        {
            break;
        }

        ctx->ev.handler = __on_fetch_timeout;
        ctx->ev.log = mcf->log;
        ctx->ev.data = ctx;
        ngx_add_timer(&ctx->ev, mcf->fetch_timeout);

        return ctx;
    } while (0);

    if (ctx->pool)
    {
        ngx_destroy_pool(ctx->pool);
    }
    __reuse_item(ctx, &g_free_ctx);
    return NULL;
}

static void __fetch_stat(autost_ctx_cache_t * ctx)
{
    if (!ctx)
    {
        ngx_http_fetch_log("ngx_http_fetch::__fetch_stat ctx is null");
        return;
    }

    static ngx_http_fetch_header_t headers[] = {
        { ngx_string("Connection"), ngx_string("Keep-Alive") },
        { ngx_string("Content-Type"), ngx_string("text/plain") }
    };
    static size_t headers_len = sizeof(headers) / sizeof(ngx_http_fetch_header_t);
    ngx_http_auth_basic_key_t auth = { g_mcf->username, g_mcf->password  };

    ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
    ngx_http_upstream_rr_peer_t * peer = peers->peer;
    while (peer)
    {
        ngx_http_fetch_args_t args = {
            NGX_HTTP_GET,
            { peer->sockaddr, peer->socklen, &peer->name, peer },
            g_mcf->autost_uri,
            ngx_null_string,
            { headers, headers_len },
            ngx_null_string,
            { NULL, NULL },
            { __post_upstream, ctx }
        };
        ngx_int_t rc = ngx_http_fetch(&args, &auth);
        if (NGX_OK != rc)
        {
            ngx_http_fetch_log("ngx_http_fetch::__fetch_stat ngx_http_fetch fail");
            --ctx->backend_stats.max_size;
            --ctx->requests;
        }
        peer = peer->next;
    }
    if (ctx->requests < 1)
    {
        ngx_http_fetch_log("ngx_http_fetch::__fetch_stat ngx_http_fetch all fail");
        ngx_del_timer(&ctx->ev);
        if (ctx->pool)
        {
            ngx_destroy_pool(ctx->pool);
        }
        __reuse_item(ctx, &g_free_ctx);
    }
}

static void __active_autost(ngx_event_t * ev)
{
    __fetch_stat(__get_autost_ctx(g_mcf));
    ngx_add_timer(&g_ev_event, g_mcf->autost_interval);
}

void hustmq_ha_invoke_autost(ngx_log_t * log)
{
    ngx_http_fetch_log("ngx_http_fetch::hustmq_ha_invoke_autost");
    ngx_memzero(&g_ev_event, sizeof(ngx_event_t));
    g_ev_event.handler = __active_autost;
    g_ev_event.log = log;

    if (g_mcf->autost_interval > 0)
    {
        ngx_add_timer(&g_ev_event, g_mcf->autost_interval);
    }
}
