#include "hustdb_ha_handler.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
    // TODO: add your fields here
} hustdb_ha_sweep_ctx_t;

static ngx_bool_t __check_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    // TODO: you can check the parameter of request here
    // char * val = ngx_http_get_param_val(&r->args, "queue", r->pool);
    return true;
}


static ngx_int_t __post_subrequest_handler(
    ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_sweep_ctx_t * ctx = ngx_http_get_addon_module_ctx(r->parent);
    if (ctx && NGX_HTTP_OK == r->headers_out.status)
    {
        ctx->base.response.len = ngx_http_get_buf_size(
            &r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;
        // TODO: you can process the response from backend server here
    }
    return ngx_http_finish_subrequest(r);
}

static ngx_int_t __first_sweep_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    if (!__check_parameter(backend_uri, r))
    {
        return NGX_ERROR;
    }

    ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
    if (!peers || !peers->peer)
    {
        return NGX_ERROR;
    }

    hustdb_ha_sweep_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_sweep_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(hustdb_ha_sweep_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);
    // TODO: you can initialize ctx here

    ctx->peer = ngx_http_first_peer(peers->peer);

    return ngx_http_gen_subrequest(backend_uri, r, ctx->peer,
        &ctx->base, __post_subrequest_handler);
}

ngx_int_t hustdb_ha_sweep_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_sweep_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __first_sweep_handler(backend_uri, r);
    }
    ctx->peer = ngx_http_next_peer(ctx->peer);
    if (ctx->peer)
    {
        return ngx_http_run_subrequest(r, &ctx->base, ctx->peer);
    }
    // TODO: you decide the return value
    return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
}