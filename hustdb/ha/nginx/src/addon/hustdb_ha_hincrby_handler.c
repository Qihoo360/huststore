#include "hustdb_ha_handler.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
    ngx_str_t host;
} hustdb_ha_hincrby_ctx_t;

static ngx_bool_t __check_parameter(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static const ngx_str_t TB = ngx_string("tb");
    static const ngx_str_t VAL = ngx_string("val");

    ngx_str_t val = ngx_null_string;
    if (NGX_OK != ngx_http_arg(r, TB.data, TB.len, &val))
    {
        return false;
    }
    if (NGX_OK != ngx_http_arg(r, VAL.data, VAL.len, &val))
    {
        return false;
    }
    return true;
}

ngx_bool_t ngx_http_append_arg(const ngx_str_t * key, const ngx_str_t * val, ngx_http_request_t *r)
{
    if (!key || !val || !key->data || !val->data || !r || !r->args.data)
    {
        return false;
    }

    static ngx_str_t TAG_AND = ngx_string("&");
    static ngx_str_t TAG_EQ = ngx_string("=");

    const ngx_str_t * arglist[] = { &r->args, &TAG_AND, key, &TAG_EQ, val };
    size_t arglist_size = sizeof(arglist) / sizeof(ngx_str_t *);

    size_t size = 0;
    size_t i = 0;
    for (i = 0; i < arglist_size; ++i)
    {
        size += arglist[i]->len;
    }

    ngx_str_t args;
    args.data = ngx_palloc(r->pool, size);
    if (!args.data)
    {
        return false;
    }
    memset(args.data, 0, size);

    size_t off = 0;
    for (i = 0; i < arglist_size; ++i)
    {
        memcpy(args.data + off, arglist[i]->data, arglist[i]->len);
        off += arglist[i]->len;
    }
    args.len = size;
    return true;
}

static hustdb_ha_hincrby_ctx_t * __create_ctx(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_hincrby_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_hincrby_ctx_t));
    if (!ctx)
    {
        return NULL;
    }

    memset(ctx, 0, sizeof(hustdb_ha_hincrby_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);

    char * key = ngx_http_get_param_val(&r->args, "key", r->pool);
    if (!key)
    {
        return NULL;
    }
    ngx_http_subrequest_peer_t * master1 = hustdb_ha_get_writelist(key);
    if (!master1)
    {
        return NULL;
    }
    ngx_http_subrequest_peer_t * master2 = master1->next;

    ngx_bool_t master1_alive = ngx_http_peer_is_alive(master1->peer);
    ngx_bool_t master2_alive = ngx_http_peer_is_alive(master2->peer);
    if (!master1_alive && !master2_alive)
    {
        return NULL;
    }
    ctx->peer = master1->peer;
    ctx->host = master2->peer->server;
    if (!master1_alive)
    {
        ctx->peer = master2->peer;
        ctx->host = master1->peer->server;
    }

    ctx->base.backend_uri = backend_uri;
    return ctx;
}

static ngx_int_t __start_hincrby(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    if (!__check_parameter(backend_uri, r))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }

    hustdb_ha_hincrby_ctx_t * ctx = __create_ctx(backend_uri, r);
    if (!ctx)
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }

    static ngx_str_t KEY = ngx_string("host");
    if (!ngx_http_append_arg(&KEY, &ctx->host, r))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }

    return ngx_http_gen_subrequest(backend_uri, r, ctx->peer,
        &ctx->base, ngx_http_post_subrequest_handler);
}

ngx_int_t hustdb_ha_hincrby_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_hincrby_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __start_hincrby(backend_uri, r);
    }
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }
    return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
}
