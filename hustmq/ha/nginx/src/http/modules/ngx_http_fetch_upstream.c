#include "ngx_http_fetch_upstream.h"
#include "ngx_http_fetch_keepalive.h"
#include "ngx_http_fetch_upstream_handler.h"
#include "ngx_http_fetch_cache.h"

static ngx_queue_t g_cache_conn = { 0, 0 };
static ngx_queue_t g_free_conn = { 0, 0 };

ngx_int_t ngx_http_fetch_upstream_init(size_t cache_size, ngx_conf_t *cf)
{
    return ngx_http_fetch_conn_cache_init(cache_size, cf->pool, &g_cache_conn, &g_free_conn);
}

static ngx_http_log_ctx_t * __create_log_ctx(ngx_connection_t * c, ngx_http_request_t * r)
{
    ngx_http_log_ctx_t * log_ctx = ngx_pcalloc(c->pool, sizeof(ngx_http_log_ctx_t));
    if (!log_ctx)
    {
        return NULL;
    }
    log_ctx->connection = c;
    log_ctx->request = r;
    log_ctx->current_request = r;
    return log_ctx;
}

static ngx_log_t * __create_log(const ngx_connection_t * c, ngx_cycle_t * cycle, ngx_http_log_ctx_t * log_ctx)
{
    ngx_log_t * log = ngx_pcalloc(c->pool, sizeof(ngx_log_t));
    if (!log)
    {
        return NULL;
    }
    log->action = "ngx_http_fetch";
    log->data = log_ctx;
    log->file = cycle->new_log.file;
    log->log_level = NGX_LOG_NOTICE;
    return log;
}

static ngx_chain_t * __send_chain(ngx_connection_t *c, ngx_chain_t *in,  off_t limit)
{
    return NULL;
}

static ngx_int_t __init_connection(ngx_http_request_t * r, ngx_log_t * log, ngx_connection_t * c)
{
    c->log = log;
    c->log_error = NGX_ERROR_INFO;
    c->pool->log = log;
    c->fd = -1;
    c->data = r;
    c->send_chain = __send_chain; // reference: ngx_linux_sendfile_chain

    // used by ngx_http_upstream_init
    c->read = ngx_pcalloc(c->pool, sizeof(ngx_event_t));
    if (!c->read)
    {
        return NGX_ERROR;
    }
    c->read->log = log;

    c->write = ngx_pcalloc(c->pool, sizeof(ngx_event_t));
    if (!c->write)
    {
        return NGX_ERROR;
    }
    c->write->log = log;
    c->write->active = 1;
    return NGX_OK;
}

static ngx_int_t __init_request(
    ngx_uint_t variables,
    ngx_http_conf_ctx_t * ctx,
    ngx_log_t * log,
    ngx_connection_t * c,
    ngx_http_request_t * r)
{
    // reference: ngx_http_core_module.c:ngx_http_subrequest
    r->signature = NGX_HTTP_MODULE;
    r->pool->log = log;
    r->main = r;
    r->connection = c;
    r->count = 1;
    r->ctx = ngx_pcalloc(r->pool, sizeof(void*) * ngx_http_max_module);
    if (!r->ctx)
    {
        return NGX_ERROR;
    }
    r->variables = ngx_pcalloc(r->pool, variables * sizeof(ngx_http_variable_value_t));
    if (!r->variables)
    {
        return NGX_ERROR;
    }
    r->header_in = ngx_create_temp_buf(r->pool, 1);
    if (!r->header_in->pos)
    {
        return NGX_ERROR;
    }
    if (ngx_list_init(&r->headers_out.headers, r->pool, 20, sizeof(ngx_table_elt_t)) != NGX_OK)
    {
        return NGX_ERROR;
    }
    r->main_conf = ctx->main_conf;
    r->srv_conf = ctx->srv_conf;
    r->loc_conf = ctx->loc_conf;
    ngx_http_clear_content_length(r);
    ngx_http_clear_accept_ranges(r);
    ngx_http_clear_last_modified(r);
    return NGX_OK;
}

static ngx_int_t __initialize(
    ngx_uint_t variables,
    ngx_http_conf_ctx_t * ctx,
    ngx_cycle_t * cycle,
    ngx_connection_t * c,
    ngx_http_request_t * r)
{
    ngx_http_log_ctx_t * log_ctx = __create_log_ctx(c, r);
    if (!log_ctx)
    {
        return NGX_ERROR;
    }
    ngx_log_t * log = __create_log(c, cycle, log_ctx);
    if (!log)
    {
        return NGX_ERROR;
    }
    if (NGX_OK != __init_connection(r, log, c))
    {
        return NGX_ERROR;
    }
    if (NGX_OK != __init_request(variables, ctx, log, c, r))
    {
        return NGX_ERROR;
    }
    return NGX_OK;
}

ngx_http_request_t * ngx_http_fetch_create_request(
    size_t request_pool_size,
    ngx_log_t * log,
    ngx_uint_t variables,
    ngx_http_conf_ctx_t * ctx,
    ngx_cycle_t * cycle)
{
    ngx_http_fetch_conn_cache_t * cache = ngx_http_get_free_connection(&g_cache_conn, &g_free_conn);
    if (!cache)
    {
        ngx_http_fetch_log("ngx_http_fetch::ngx_http_fetch_create_request::no free connection");
        return NULL;
    }

    do
    {
        if (NGX_OK != ngx_http_fetch_create_pool(request_pool_size, log, cache))
        {
            ngx_http_fetch_log("ngx_http_fetch::ngx_http_fetch_create_request::ngx_http_fetch_create_pool fail");
            break;
        }
        ngx_http_request_t * r = &cache->request;
        ngx_connection_t * c = &cache->connection;

        memset(r, 0, sizeof(ngx_http_request_t));
        memset(c, 0, sizeof(ngx_connection_t));

        r->pool = cache->request_pool;
        c->pool = cache->conn_pool;

        if (NGX_OK != __initialize(variables, ctx, cycle, c, r))
        {
            ngx_http_fetch_log("ngx_http_fetch::ngx_http_fetch_create_request::initialize fail");
            break;
        }
        return r;
    } while (0);

    ngx_http_fetch_reuse_connection(&cache->connection, &g_cache_conn, &g_free_conn);
    return NULL;
}

static ngx_int_t __init_upstream(ngx_str_t * schema, ngx_http_upstream_conf_t * umcf, ngx_http_request_t *r)
{
    ngx_http_upstream_t *u = ngx_pcalloc(r->pool, sizeof(ngx_http_upstream_t));
    if (!u)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->upstream = u;
    u->schema = *schema;
    u->conf = umcf;

    u->peer.log = r->connection->log;
    u->peer.log_error = NGX_ERROR_ERR;

    u->create_request = ngx_http_fetch_upstream_create_request;
    u->reinit_request = ngx_http_fetch_upstream_reinit_request;
    u->process_header = ngx_http_fetch_upstream_process_header;
    u->abort_request = ngx_http_fetch_upstream_abort_request;
    u->finalize_request = ngx_http_fetch_upstream_finalize_request;

    u->request_sent = 0;
    u->header_sent = 0;

    u->buffering = 1;

    // reference: ngx_http_proxy_handler

    u->pipe = ngx_pcalloc(r->pool, sizeof(ngx_event_pipe_t));
    if (!u->pipe)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    u->pipe->input_filter = ngx_http_fetch_upstream_copy_filter;
    u->pipe->input_ctx = r;

    u->input_filter_init = ngx_http_fetch_upstream_input_filter_init;
    u->input_filter = ngx_http_fetch_upstream_non_buffered_copy_filter;
    u->input_filter_ctx = r;

    return NGX_OK;
}

static ngx_int_t __reuse_connection(ngx_connection_t * c)
{
    return ngx_http_fetch_reuse_connection(c, &g_cache_conn, &g_free_conn);
}

static ngx_int_t __adjust_connection(ngx_connection_t * c)
{
    return ngx_http_fetch_adjust_connection(c, &g_cache_conn);
}

ngx_int_t ngx_http_fetch_init_upstream(const ngx_http_fetch_upstream_data_t * data)
{
    ngx_http_request_t *r = data->r;
    if (NGX_OK != __init_upstream(data->schema, data->umcf, r))
    {
        return NGX_ERROR;
    }

    ngx_http_fetch_ctx_t * ctx;
    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_fetch_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(ngx_http_fetch_ctx_t));
    ngx_http_fetch_set_module_ctx(r, ctx);

    memcpy(&ctx->addr, &data->args->addr, sizeof(ngx_addr_t));
    memcpy(&ctx->headers, &data->args->headers, sizeof(ngx_http_fetch_headers_t));
    ctx->headers_in_hash = data->headers_in_hash;
    ctx->auth = data->auth;
    ctx->post_upstream = data->args->post_upstream;
    if (data->args->body.data && data->args->body.len > 0)
    {
        ctx->body = ngx_http_fetch_create_buf(&data->args->body, r->pool);
    }
    else
    {
        ctx->body = NULL;
    }
    ctx->reuse = __reuse_connection;
    ctx->adjust = __adjust_connection;

    ngx_http_upstream_init(r);
    return NGX_OK;
}
