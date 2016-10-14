#include "hustdb_ha_write_inner.h"

ngx_bool_t hustdb_ha_has_tb(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static ngx_str_t arg = ngx_string("tb");
    ngx_str_t rc = ngx_null_string;
    return NGX_OK == ngx_http_arg(r, arg.data, arg.len, &rc);
}

ngx_bool_t hustdb_ha_has_ttl(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    static ngx_str_t arg = ngx_string("ttl");
    ngx_str_t rc = ngx_null_string;
    return NGX_OK == ngx_http_arg(r, arg.data, arg.len, &rc);
}

ngx_bool_t hustdb_ha_check_incr(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    if (!hustdb_ha_has_tb(backend_uri, r))
    {
        return false;
    }
    static ngx_str_t arg = ngx_string("val");
    ngx_str_t rc = ngx_null_string;
    return NGX_OK == ngx_http_arg(r, arg.data, arg.len, &rc);
}

static void __post_handler(ngx_http_request_t *r)
{
    --r->main->count;

    hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    ngx_bool_t rc = true;
    if (ctx->base.key_in_body)
    {
        if (!hustdb_ha_init_write_ctx_by_body(r, ctx))
        {
            rc = false;
        }
    }

    if (!rc)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        return;
    }

    ngx_http_gen_subrequest(
        ctx->base.base.backend_uri,
        r,
        ctx->base.peer->peer,
        &ctx->base.base,
        hustdb_ha_on_subrequest_complete);
}

ngx_int_t hustdb_ha_start_post(
    ngx_bool_t support_post_only,
    ngx_bool_t key_in_body,
    ngx_bool_t has_tb,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_write_ctx_t * ctx = hustdb_ha_create_write_ctx(r);
    if (!ctx)
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    ctx->base.base.backend_uri = backend_uri;

    if (support_post_only && !(r->method & NGX_HTTP_POST))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }

    if (key_in_body)
    {
        ctx->base.key_in_body = true;
        ctx->base.has_tb = has_tb;
    }
    else
    {
        if (!hustdb_ha_parse_args(has_tb, r, ctx))
        {
            return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        }
    }

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

ngx_int_t hustdb_ha_start_del(
    ngx_bool_t support_post_only,
    ngx_bool_t key_in_body,
    ngx_bool_t has_tb,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    if (key_in_body)
    {
        return hustdb_ha_start_post(support_post_only, key_in_body, has_tb, backend_uri, r);
    }

    hustdb_ha_write_ctx_t * ctx = hustdb_ha_create_write_ctx(r);
    if (!ctx)
    {
        return NGX_ERROR;
    }
    ctx->base.base.backend_uri = backend_uri;

    if (!hustdb_ha_parse_args(has_tb, r, ctx))
    {
        return NGX_ERROR;
    }

    return ngx_http_gen_subrequest(
            ctx->base.base.backend_uri,
            r,
            ctx->base.peer->peer,
            &ctx->base.base,
            hustdb_ha_on_subrequest_complete);
}

static ngx_bool_t __match_method(uint8_t methods[], size_t size, uint8_t method)
{
    size_t i = 0;
    for (i = 0; i < size; ++i)
    {
        if (method == methods[i])
        {
            return true;
        }
    }
    return false;
}

static ngx_bool_t __skip_not_found_error(uint8_t method, ngx_uint_t status)
{
    if (NGX_HTTP_NOT_FOUND != status)
    {
        return false;
    }
    static uint8_t methods[] = {
        HUSTDB_METHOD_DEL,
        HUSTDB_METHOD_HDEL,
        HUSTDB_METHOD_SREM,
        HUSTDB_METHOD_ZREM,
    };
    static size_t size = sizeof(methods) / sizeof(uint8_t);
    return __match_method(methods, size, method);
}

static ngx_bool_t __skip_precondition_error(uint8_t method, ngx_uint_t status)
{
    if (NGX_HTTP_PRECONDITION_FAILED != status)
    {
        return false;
    }
    static uint8_t methods[] = {
        HUSTDB_METHOD_DEL,
        HUSTDB_METHOD_HDEL,
        HUSTDB_METHOD_SREM,
        HUSTDB_METHOD_ZREM,
        HUSTDB_METHOD_HSET,
        HUSTDB_METHOD_SADD,
        HUSTDB_METHOD_ZADD
    };
    static size_t size = sizeof(methods) / sizeof(uint8_t);
    return __match_method(methods, size, method);
}

static ngx_bool_t __skip_error(uint8_t method, ngx_uint_t status)
{
    return __skip_precondition_error(method, status)
        || __skip_not_found_error(method, status);
}

static void __update_error(uint8_t method, ngx_uint_t status, hustdb_ha_write_ctx_t * ctx)
{
    if (NGX_HTTP_OK == status)
    {
        ctx->health_peer = ctx->base.peer;
        return;
    }
    if (__skip_error(method, status))
    {
        ++ctx->skip_error_count;
        return;
    }
    ++ctx->error_count;
    ctx->error_peer = ctx->base.peer;
}

// -------------------------------------------------------------
// |skip   |  0   |  1   |  2   |  0       |  1   |  2  |  0   |
// -------------------------------------------------------------
// |err    |  0   |  0   |  0   |  1       |  1   |  0  |  2   |
// -------------------------------------------------------------
// |action |  200 |  200 |  404 |  200&log |  404 | 404 | 404  |
// -------------------------------------------------------------
static ngx_int_t __post_write_data(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
{
    if (0 == ctx->skip_error_count && 1 == ctx->error_count)
    {
        return hustdb_ha_write_binlog(method, has_tb, r, ctx);
    }
    if (0 == ctx->skip_error_count && 0 == ctx->error_count)
    {
        return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->base.version, NULL, r);
    }
    return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
}

static ngx_int_t __on_write_master1_complete(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
{
	__update_error(method, r->headers_out.status, ctx);
	ctx->base.peer = ctx->base.peer->next;
	ngx_bool_t alive = ngx_http_peer_is_alive(ctx->base.peer->peer);
	ngx_http_hustdb_ha_main_conf_t * mcf = hustdb_ha_get_module_main_conf(r);
	if (mcf->debug_sync)
	{
	    alive = false;
	}
	if (alive) // master2
	{
		ctx->state = STATE_WRITE_MASTER2;
		return ngx_http_run_subrequest(r, &ctx->base.base, ctx->base.peer->peer);
	}

	// master2 dead
	++ctx->error_count;
	ctx->error_peer = ctx->base.peer;

	return __post_write_data(method, has_tb, r, ctx);
}

static ngx_int_t __on_write_master2_complete(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
{
    __update_error(method, r->headers_out.status, ctx);
    return __post_write_data(method, has_tb, r, ctx);
}

static ngx_int_t __on_write_binlog_complete(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
{
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        return hustdb_ha_write_sync_data(method, has_tb, r, ctx);
    }
    if (!hustdb_ha_add_sync_head(&ctx->error_peer->peer->server, r))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->base.version, NULL, r);
}

static ngx_int_t __switch_state(
    uint8_t method,
    ngx_bool_t has_tb,
    ngx_http_request_t *r,
    hustdb_ha_write_ctx_t * ctx)
{
    if (STATE_WRITE_MASTER1 == ctx->state)
    {
        return __on_write_master1_complete(method, has_tb, r, ctx);
    }
    else if (STATE_WRITE_MASTER2 == ctx->state)
    {
        return __on_write_master2_complete(method, has_tb, r, ctx);
    }
    else if (STATE_WRITE_BINLOG == ctx->state)
    {
        return __on_write_binlog_complete(method, has_tb, r, ctx);
    }
    return NGX_ERROR;
}

ngx_int_t hustdb_ha_write_handler(
    uint8_t method,
    ngx_bool_t support_post_only,
    ngx_bool_t key_in_body,
    ngx_bool_t has_tb,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r,
    hustdb_ha_start_write_t start_write)
{
    hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return start_write(support_post_only, key_in_body, has_tb, backend_uri, r);
    }
    return __switch_state(method, has_tb, r, ctx);
}

static hustdb_ha_write_ctx_t * __create_zwrite_ctx(ngx_http_request_t *r)
{
    hustdb_ha_write_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_write_ctx_t));
    if (!ctx)
    {
        return NULL;
    }
    ngx_http_set_addon_module_ctx(r, ctx);
    memset(ctx, 0, sizeof(hustdb_ha_write_ctx_t));
    return ctx;
}

static void __post_body_handler(ngx_http_request_t *r)
{
    --r->main->count;

    hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    char * key = hustdb_ha_get_key_from_body(r);
    if (key)
    {
        ctx->base.key_in_body = true;
    }
    else
    {
        key = ngx_http_get_param_val(&r->args, "key", r->pool);
    }

    ctx->base.key = key;
    if (!ctx->base.key)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        return;
    }

    ngx_http_gen_subrequest(
        ctx->base.base.backend_uri,
        r,
        ctx->base.peer->peer,
        &ctx->base.base,
        hustdb_ha_on_subrequest_complete);
}

static ngx_int_t __start_zwrite(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_write_ctx_t * ctx = __create_zwrite_ctx(r);
    if (!ctx)
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    ctx->base.base.backend_uri = backend_uri;

    if (!hustdb_ha_parse_zset_args(r, ctx))
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

ngx_int_t hustdb_ha_zwrite_handler(
    uint8_t method,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_write_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __start_zwrite(backend_uri, r);
    }
    return __switch_state(method, true, r, ctx);
}
