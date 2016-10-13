#include "hustdb_ha_handler_inner.h"

typedef enum
{
    STATE_READ_MASTER1,
    STATE_READ_MASTER2
} hustdb_read_state_t;

typedef struct
{
    ngx_str_t version;
    ngx_str_t data;
} hustdb_read_response_t;

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_subrequest_peer_t * peer;
    hustdb_read_state_t state;
    ngx_str_t version;
    hustdb_read_response_t master1_resp;
    hustdb_read_response_t master2_resp;
} hustdb_ha_read_ctx_t;

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
