#include "hustdb_ha_handler_base.h"

static hustdb_ha_ctx_t * __create_ctx(ngx_http_request_t *r)
{
    hustdb_ha_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_ctx_t));
    if (!ctx)
    {
        return NULL;
    }
    ngx_http_set_addon_module_ctx(r, ctx);
    memset(ctx, 0, sizeof(hustdb_ha_ctx_t));
    return ctx;
}

ngx_http_subrequest_peer_t * hustdb_ha_hash_peer(const char * arg, ngx_http_request_t *r)
{
    char * key = ngx_http_get_param_val(&r->args, arg, r->pool);
    if (!key)
    {
        return NULL;
    }
    return hustdb_ha_get_readlist(key);
}

static ngx_http_subrequest_peer_t * __get_peer_by_key(ngx_http_request_t *r)
{
    char * key = ngx_http_get_param_val(&r->args, "key", r->pool);
    if (!key)
    {
        return NULL;
    }
    return ngx_http_get_first_peer(hustdb_ha_get_readlist(key));
}

static ngx_int_t __start_read(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	ngx_http_subrequest_peer_t * peer = __get_peer_by_key(r);
	if (!peer)
	{
		return NGX_ERROR;
	}

	hustdb_ha_ctx_t * ctx = __create_ctx(r);
	if (!ctx)
	{
		return NGX_ERROR;
	}

	ctx->peer = peer;
	return ngx_http_gen_subrequest(
	        backend_uri,
			r,
			peer->peer,
			&ctx->base,
			hustdb_ha_on_subrequest_complete);
}

static ngx_http_subrequest_peer_t * __get_readable_peer(ngx_bool_t key_in_body, ngx_http_request_t *r)
{
    if (key_in_body)
    {
        char * key = hustdb_ha_get_key_from_body(r);
        if (!key)
        {
            return NULL;
        }
        return ngx_http_get_first_peer(hustdb_ha_get_readlist(key));
    }
    return __get_peer_by_key(r);
}

static void __post_handler(ngx_http_request_t *r)
{
    --r->main->count;
    hustdb_ha_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    ngx_http_subrequest_peer_t * peer = __get_readable_peer(ctx->key_in_body, r);
    if (!peer)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    else
    {
        ctx->peer = peer;

        ngx_http_gen_subrequest(
            ctx->base.backend_uri,
            r,
            peer->peer,
            &ctx->base,
            hustdb_ha_on_subrequest_complete);
    }
}

static ngx_int_t __post_read(ngx_bool_t key_in_body, ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_ctx_t * ctx = __create_ctx(r);
    if (!ctx)
    {
        return NGX_ERROR;
    }
    ctx->base.backend_uri = backend_uri;
    ctx->key_in_body = key_in_body;

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

ngx_int_t hustdb_ha_read_handler(
    ngx_bool_t read_body,
    ngx_bool_t key_in_body,
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
	hustdb_ha_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
	if (!ctx)
	{
	    if (read_body && !(r->method & NGX_HTTP_POST))
	    {
	        return NGX_HTTP_NOT_ALLOWED;
	    }
	    if (check_parameter && !check_parameter(backend_uri, r))
        {
            return NGX_ERROR;
        }
	    return read_body ? __post_read(key_in_body, backend_uri, r) : __start_read(backend_uri, r);
	}
	if (NGX_HTTP_OK != r->headers_out.status)
	{
		ctx->peer = ngx_http_get_next_peer(ctx->peer);
		return ctx->peer ? ngx_http_run_subrequest(
				r, &ctx->base, ctx->peer->peer) : hustdb_ha_send_response(
						NGX_HTTP_NOT_FOUND, NULL, NULL, r);
	}
	return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->version, &ctx->base.response, r);
}

static void __post_body_handler(ngx_http_request_t *r)
{
    --r->main->count;
    hustdb_ha_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    ctx->tb = ngx_http_get_param_val(&r->args, "tb", r->pool);
    ctx->key = hustdb_ha_get_key(r);
    if (!ctx->key || !ctx->tb)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        return;
    }

    ngx_http_subrequest_peer_t * peer =  ngx_http_get_first_peer(hustdb_ha_get_readlist(ctx->tb));
    if (!peer)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    else
    {
        ctx->peer = peer;

        ngx_http_gen_subrequest(
            ctx->base.backend_uri,
            r,
            peer->peer,
            &ctx->base,
            hustdb_ha_on_subrequest_complete);
    }
}

ngx_int_t hustdb_ha_zread_handler(
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        if (check_parameter && !check_parameter(backend_uri, r))
        {
            return NGX_ERROR;
        }
        hustdb_ha_ctx_t * ctx = __create_ctx(r);
        if (!ctx)
        {
            return NGX_ERROR;
        }
        ctx->base.backend_uri = backend_uri;

        ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
        if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
        {
            return rc;
        }
        return NGX_DONE;
    }
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        ctx->peer = ngx_http_get_next_peer(ctx->peer);
        return ctx->peer ? ngx_http_run_subrequest(
                r, &ctx->base, ctx->peer->peer) : hustdb_ha_send_response(
                        NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    return hustdb_ha_send_response(NGX_HTTP_OK, &ctx->version, &ctx->base.response, r);
}

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_subrequest_peer_t * peer;
    const ngx_str_t * keys;
} zkeys_ctx_t;

static ngx_int_t __on_subrequest_complete(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    zkeys_ctx_t * ctx = data;
    if (ctx && NGX_HTTP_OK == r->headers_out.status)
    {
        ctx->base.response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;
        ctx->keys = hustdb_ha_get_keys_from_header(r);
    }
    return ngx_http_finish_subrequest(r);
}

static void __zkeys_post_body_handler(ngx_http_request_t *r)
{
    --r->main->count;
    zkeys_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);

    char * tb = ngx_http_get_param_val(&r->args, "tb", r->pool);
    if (!tb)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
        return;
    }

    ngx_http_subrequest_peer_t * peer =  ngx_http_get_first_peer(hustdb_ha_get_readlist(tb));
    if (!peer)
    {
        hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    else
    {
        ctx->peer = peer;

        ngx_http_gen_subrequest(
            ctx->base.backend_uri,
            r,
            peer->peer,
            &ctx->base,
            __on_subrequest_complete);
    }
}

ngx_int_t hustdb_ha_zread_keys_handler(
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    zkeys_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        if (check_parameter && !check_parameter(backend_uri, r))
        {
            return NGX_ERROR;
        }
        zkeys_ctx_t * ctx = ngx_palloc(r->pool, sizeof(zkeys_ctx_t));
        if (!ctx)
        {
            return NGX_ERROR;
        }
        ngx_http_set_addon_module_ctx(r, ctx);
        memset(ctx, 0, sizeof(zkeys_ctx_t));
        ctx->base.backend_uri = backend_uri;

        ngx_int_t rc = ngx_http_read_client_request_body(r, __zkeys_post_body_handler);
        if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
        {
            return rc;
        }
        return NGX_DONE;
    }
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        ctx->peer = ngx_http_get_next_peer(ctx->peer);
        return ctx->peer ? ngx_http_run_subrequest(
                r, &ctx->base, ctx->peer->peer) : hustdb_ha_send_response(
                        NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }
    if (ctx->keys)
    {
        if (!hustdb_ha_add_keys_to_header(ctx->keys, r))
        {
            return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
        }
    }
    return ngx_http_send_response_imp(r->headers_out.status, &ctx->base.response, r);
}


static ngx_int_t __on_subrequest_complete2(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_read_ctx_t * ctx = data;
    do
    {
        if (!ctx || NGX_HTTP_OK != r->headers_out.status)
        {
            break;
        }

        ctx->base.response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;

        ngx_str_t * val = hustdb_ha_get_version(r);
        if (val)
        {
            ctx->version = hustdb_ha_make_str(val, r->parent);
        }

    } while (0);
    return ngx_http_finish_subrequest(r);
}

static void __post_body_handler2(ngx_http_request_t *r)
{
    --r->main->count;
    hustdb_ha_read_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    ngx_http_gen_subrequest(
        ctx->base.backend_uri,
        r,
        ctx->peer->peer,
        &ctx->base,
        __on_subrequest_complete2);
}

static ngx_int_t __start_read2(const char * arg, ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_subrequest_peer_t * peer = hustdb_ha_hash_peer(arg, r);
    if (!peer)
    {
        return NGX_ERROR;
    }

    hustdb_ha_read_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_read_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    ngx_http_set_addon_module_ctx(r, ctx);
    memset(ctx, 0, sizeof(hustdb_ha_read_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }

    ctx->base.backend_uri = backend_uri;
    ctx->peer = peer;
    ctx->state = STATE_READ_MASTER1;

    ngx_bool_t alive = ngx_http_peer_is_alive(ctx->peer->peer);
    if (!alive) // master1
    {
        return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
    }

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler2);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

static ngx_int_t __on_read_master1_complete(ngx_http_request_t *r, hustdb_ha_read_ctx_t * ctx)
{
    if (NGX_HTTP_OK == r->headers_out.status)
    {
        ctx->peer = ctx->peer->next;
        ngx_bool_t alive = ngx_http_peer_is_alive(ctx->peer->peer);
        if (alive) // master2
        {
            // save response of master1
            ctx->master1_resp.version = ctx->version;
            ctx->master1_resp.data = hustdb_ha_make_str(&ctx->base.response, r);

            ctx->state = STATE_READ_MASTER2;
            return ngx_http_run_subrequest(r, &ctx->base, ctx->peer->peer);
        }
    }
    return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
}

static ngx_bool_t __string_eq(const ngx_str_t * src, const ngx_str_t * dst)
{
    if (src->len != dst->len)
    {
        return false;
    }
    return 0 == ngx_strncmp(src->data, dst->data, src->len);
}

static ngx_bool_t __add_conflict_ver(const ngx_str_t * ver1, const ngx_str_t * ver2, ngx_http_request_t *r)
{
    static ngx_str_t VER1 = ngx_string("Version1");
    static ngx_str_t VER2 = ngx_string("Version2");

    if (!ngx_http_add_field_to_headers_out(&VER1, ver1, r))
    {
        return false;
    }
    if (!ngx_http_add_field_to_headers_out(&VER2, ver2, r))
    {
        return false;
    }
    return true;
}

static ngx_bool_t __add_val_offset(uint64_t off, ngx_http_request_t *r)
{
    static ngx_str_t OFF = ngx_string("Val-Offset");

    ngx_str_t tmp;
    tmp.data = ngx_palloc(r->pool, 21);
    sprintf((char *)tmp.data, "%lu", off);
    tmp.len = strlen((const char *)tmp.data);

    return ngx_http_add_field_to_headers_out(&OFF, &tmp, r);
}

static ngx_bool_t __add_conflict_val(const ngx_str_t * val1, const ngx_str_t * val2, ngx_http_request_t *r, ngx_str_t * val)
{
    if (!val1->data || !val2->data || val1->len < 1 || val2->len < 1)
    {
        return false;
    }

    if (!__add_val_offset(val1->len, r))
    {
        return false;
    }

    val->len = val1->len + val2->len;
    val->data = ngx_palloc(r->pool, val->len);
    if (!val->data)
    {
        return false;
    }

    memcpy(val->data, val1->data, val1->len);
    memcpy(val->data + val1->len, val2->data, val2->len);

    return true;
}

static ngx_int_t __on_read_master2_complete(ngx_http_request_t *r, hustdb_ha_read_ctx_t * ctx)
{
    do
    {
        if (NGX_HTTP_OK != r->headers_out.status)
        {
            break;
        }
        // save response of master2
        ctx->master2_resp.version = ctx->version;
        ctx->master2_resp.data = ctx->base.response;

        ngx_bool_t ver_eq = __string_eq(&ctx->master1_resp.version, &ctx->master2_resp.version);
        if (!ver_eq && !__add_conflict_ver(&ctx->master1_resp.version, &ctx->master2_resp.version, r))
        {
            break;
        }
        if (ver_eq && !hustdb_ha_add_version(&ctx->version, r))
        {
            break;
        }
        ngx_bool_t val_eq = __string_eq(&ctx->master1_resp.data, &ctx->master2_resp.data);
        ngx_str_t response = { 0, 0 };
        if (!val_eq && !__add_conflict_val(&ctx->master1_resp.data, &ctx->master2_resp.data, r, &response))
        {
            break;
        }

        return val_eq ? ngx_http_send_response_imp(
            NGX_HTTP_OK, &ctx->base.response, r) : ngx_http_send_response_imp(NGX_HTTP_CONFLICT, &response, r);

    } while (0);

    return hustdb_ha_send_response(NGX_HTTP_NOT_FOUND, NULL, NULL, r);
}

ngx_int_t hustdb_ha_read2_handler(const char * arg, ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustdb_ha_read_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __start_read2(arg, backend_uri, r);
    }
    if (STATE_READ_MASTER1 == ctx->state)
    {
        return __on_read_master1_complete(r, ctx);
    }
    else if (STATE_READ_MASTER2 == ctx->state)
    {
        return __on_read_master2_complete(r, ctx);
    }
    return NGX_ERROR;
}
