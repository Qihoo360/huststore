#include "hustdb_ha_handler_inner.h"
#include "ngx_http_upstream_check_module.h"

static const ngx_str_t VERSION_KEY = ngx_string("Version");
static const ngx_str_t VERSION_VAL = ngx_string("0");

static const ngx_str_t KEYS_KEY = ngx_string("Keys");

ngx_int_t hustdb_ha_send_response(
    ngx_uint_t status,
    const ngx_str_t * version,
    const ngx_str_t * response,
    ngx_http_request_t *r)
{
    const ngx_str_t * ver = (NGX_HTTP_OK == status) ? version : &VERSION_VAL;
    if (!hustdb_ha_add_version(ver, r))
    {
        return NGX_ERROR;
    }
    return ngx_http_send_response_imp(status, response, r);
}

ngx_str_t * hustdb_ha_get_version(ngx_http_request_t * r)
{
    return ngx_http_find_head_value(&r->headers_out.headers, &VERSION_KEY);
}

ngx_bool_t hustdb_ha_add_version(const ngx_str_t * version, ngx_http_request_t * r)
{
    return ngx_http_add_field_to_headers_out(&VERSION_KEY, version, r);
}

ngx_int_t hustdb_ha_on_subrequest_complete(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_ctx_t * ctx = data;

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
            if (!ctx->version.data)
            {
                ctx->version = hustdb_ha_make_str(val, r->parent);
            }
            else
            {
                ngx_int_t src = ngx_atoi(ctx->version.data, ctx->version.len);
                ngx_int_t dst = ngx_atoi(val->data, val->len);
                if (dst > src)
                {
                    ctx->version = hustdb_ha_make_str(val, r->parent);
                }
            }
        }

    } while (0);

    return ngx_http_finish_subrequest(r);
}

ngx_str_t * hustdb_ha_get_keys_from_header(ngx_http_request_t * r)
{
    return ngx_http_find_head_value(&r->headers_out.headers, &KEYS_KEY);
}

ngx_bool_t hustdb_ha_add_keys_to_header(const ngx_str_t * keys, ngx_http_request_t * r)
{
    return ngx_http_add_field_to_headers_out(&KEYS_KEY, keys, r);
}

static ngx_int_t __on_subrequest_complete(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_peer_ctx_t * ctx = data;
    if (ctx && NGX_HTTP_OK == r->headers_out.status)
    {
        ctx->base.response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;
        ctx->keys = hustdb_ha_get_keys_from_header(r);
    }
    return ngx_http_finish_subrequest(r);
}

static ngx_int_t __post_peer(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_upstream_rr_peer_t * peer = hustdb_ha_get_peer(r);
    if (!peer)
    {
        return NGX_ERROR;
    }

    hustdb_ha_peer_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_peer_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(hustdb_ha_peer_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);


    ngx_str_t key = ngx_string(PEER_KEY);
    ctx->base.args = ngx_http_remove_param(&r->args, &key, r->pool);

    return ngx_http_gen_subrequest(backend_uri, r, peer,
        &ctx->base, __on_subrequest_complete);
}

ngx_int_t hustdb_ha_post_peer(
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_peer_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        if (check_parameter && !check_parameter(backend_uri, r))
        {
            return NGX_ERROR;
        }
        return __post_peer(backend_uri, r);
    }

    if (ctx->keys)
    {
        if (!hustdb_ha_add_keys_to_header(ctx->keys, r))
        {
            return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
        }
    }
    return (NGX_HTTP_OK == r->headers_out.status) ? ngx_http_send_response_imp(
        r->headers_out.status, &ctx->base.response, r) : ngx_http_send_response_imp(
            NGX_HTTP_NOT_FOUND, NULL, r);
}
