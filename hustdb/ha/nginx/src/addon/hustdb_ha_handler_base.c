#include "hustdb_ha_handler_base.h"
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
    if (!ngx_http_add_field_to_headers_out(&VERSION_KEY, ver, r))
    {
        return NGX_ERROR;
    }

    return ngx_http_send_response_imp(status, response, r);
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

        ngx_str_t * val = ngx_http_find_head_value(&r->headers_out.headers, &VERSION_KEY);
        if (!ctx->version)
        {
            ctx->version = val;
        }
        else
        {
            ngx_int_t src = ngx_atoi(ctx->version->data, ctx->version->len);
            ngx_int_t des = ngx_atoi(val->data, val->len);
            if (des > src)
            {
                ctx->version = val;
            }
        }

    } while (0);

    return ngx_http_finish_subrequest(r);
}

static ngx_int_t __on_subrequest_complete(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_peer_ctx_t * ctx = data;
    if (ctx && NGX_HTTP_OK == r->headers_out.status)
    {
        ctx->base.response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;
        ctx->keys = ngx_http_find_head_value(&r->headers_out.headers, &KEYS_KEY);
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
        if (!ngx_http_add_field_to_headers_out(&KEYS_KEY, ctx->keys, r))
        {
            return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
        }
    }
    return (NGX_HTTP_OK == r->headers_out.status) ? ngx_http_send_response_imp(
        r->headers_out.status, &ctx->base.response, r) : ngx_http_send_response_imp(
            NGX_HTTP_NOT_FOUND, NULL, r);;
}


static ngx_int_t __first_loop(hustdb_ha_check_parameter_t check_parameter, ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    if (!check_parameter(backend_uri, r))
    {
        return NGX_ERROR;
    }

    ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
    if (!peers || !peers->peer)
    {
        return NGX_ERROR;
    }

    hustdb_ha_loop_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_loop_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(hustdb_ha_loop_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);

    ctx->peer = ngx_http_first_peer(peers->peer);

    return ngx_http_gen_subrequest(backend_uri, r, ctx->peer,
        &ctx->base, ngx_http_post_subrequest_handler);
}

ngx_int_t hustdb_ha_loop_handler(
    hustdb_ha_check_parameter_t check_parameter,
    ngx_str_t * backend_uri,
    ngx_http_request_t *r)
{
    hustdb_ha_loop_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __first_loop(check_parameter, backend_uri, r);
    }
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }
    ctx->peer = ngx_http_next_peer(ctx->peer);
    if (ctx->peer)
    {
        return ngx_http_run_subrequest(r, &ctx->base, ctx->peer);
    }
    return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
}

ngx_bool_t hustdb_ha_get_readable_peer(size_t buckets, size_t bucket, hustdb_ha_bucket_buf_t * result)
{
    for (result->bucket = bucket; result->bucket < buckets; ++result->bucket)
    {
        hustdb_ha_bucket_t * bk = hustdb_ha_get_bucket(result->bucket);
        if (bk)
        {
            result->peer = ngx_http_get_first_peer(bk->readlist);
            if (result->peer)
            {
                return true;
            }
        }
        ++result->bad_buckets;
    }
    return false;
}
