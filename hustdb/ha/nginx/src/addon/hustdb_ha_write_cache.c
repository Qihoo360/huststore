#include "hustdb_ha_write_inner.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_subrequest_peer_t * peer;
    hustdb_write_state_t state;
    int error_count;
    ngx_http_subrequest_peer_t * error_peer;
} hustdb_ha_write_cache_ctx_t;

static void __copy_args(const hustdb_ha_write_args_t * src, hustdb_ha_write_cache_ctx_t * dst)
{
    dst->peer = src->peer;
    dst->state = src->state;
    dst->error_count = src->error_count;
    dst->error_peer = src->error_peer;
}

static ngx_bool_t __parse_cache_args(
    hustdb_ha_check_parameter_t check,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r,
    hustdb_ha_write_cache_ctx_t * ctx)
{
    char * key = ngx_http_get_param_val(&r->args, "key", r->pool);
    if (!key)
    {
        return false;
    }

    if (check && !check(backend_uri, r))
    {
        return false;
    }

    ctx->peer = hustdb_ha_get_writelist(key);
    if (!ctx->peer)
    {
        return false;
    }

    hustdb_ha_write_args_t tmp;
    if (!hustdb_ha_init_write_args(key, &tmp))
    {
        return false;
    }
    __copy_args(&tmp, ctx);

    return true;
}

static ngx_int_t __post_write_cache(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_write_cache_ctx_t * ctx = data;

    do
    {
        if (!ctx || NGX_HTTP_OK != r->headers_out.status)
        {
            break;
        }

        ctx->base.response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;

    } while (0);

    return ngx_http_finish_subrequest(r);
}

static void __post_cache_handler(ngx_http_request_t *r)
{
    --r->main->count;
    hustdb_ha_write_cache_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    ngx_http_gen_subrequest(
        ctx->base.backend_uri,
        r,
        ctx->peer->peer,
        &ctx->base,
        __post_write_cache);
}

static ngx_int_t __start_write_cache(hustdb_ha_check_parameter_t check, ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_write_cache_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_write_cache_ctx_t));
    if (!ctx)
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    ngx_http_set_addon_module_ctx(r, ctx);
    memset(ctx, 0, sizeof(hustdb_ha_write_cache_ctx_t));
    ctx->base.backend_uri = backend_uri;

    if (!__parse_cache_args(check, backend_uri, r, ctx))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_cache_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

static void __update_cache_error(ngx_uint_t status, hustdb_ha_write_cache_ctx_t * ctx)
{
    if (NGX_HTTP_OK == status)
    {
        return;
    }
    ++ctx->error_count;
    ctx->error_peer = ctx->peer;
}

static ngx_int_t __send_write_cache_response(ngx_http_request_t *r, hustdb_ha_write_cache_ctx_t * ctx)
{
    if (0 == ctx->error_count)
    {
        return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
    }
    if (1 == ctx->error_count)
    {
        ngx_str_t * server = &ctx->error_peer->peer->server;
        static ngx_str_t FAIL_KEY = ngx_string("Fail");
        if (ngx_http_add_field_to_headers_out(&FAIL_KEY, server, r))
        {
            return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
        }
    }
    return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
}

static ngx_int_t __on_write_cache_master1_complete(ngx_http_request_t *r, hustdb_ha_write_cache_ctx_t * ctx)
{
    __update_cache_error(r->headers_out.status, ctx);
    ctx->peer = ctx->peer->next;
    ngx_bool_t alive = ngx_http_peer_is_alive(ctx->peer->peer);
    if (alive) // master2
    {
        ctx->state = STATE_WRITE_MASTER2;
        return ngx_http_run_subrequest(r, &ctx->base, ctx->peer->peer);
    }

    // master2 dead
    ++ctx->error_count;
    ctx->error_peer = ctx->peer;

    return __send_write_cache_response(r, ctx);
}

static ngx_int_t __on_write_cache_master2_complete(ngx_http_request_t *r, hustdb_ha_write_cache_ctx_t * ctx)
{
    __update_cache_error(r->headers_out.status, ctx);
    return __send_write_cache_response(r, ctx);
}

ngx_int_t hustdb_ha_write_cache_handler(
    hustdb_ha_check_parameter_t check,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_write_cache_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __start_write_cache(check, backend_uri, r);
    }
    if (STATE_WRITE_MASTER1 == ctx->state)
    {
        return __on_write_cache_master1_complete(r, ctx);
    }
    else if (STATE_WRITE_MASTER2 == ctx->state)
    {
        return __on_write_cache_master2_complete(r, ctx);
    }
    return NGX_ERROR;
}
