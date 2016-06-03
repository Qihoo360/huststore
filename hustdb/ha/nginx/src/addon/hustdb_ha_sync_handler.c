#include "hustdb_ha_sync_handler.h"

typedef struct
{
    ngx_http_request_t * r;
} hustdb_ha_sync_ctx_t;

static void __finialize(ngx_bool_t err, const ngx_str_t * response, ngx_http_request_t *r)
{
    ngx_int_t rc = 0;
    if (err)
    {
        rc = ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }
    else
    {
        rc = ngx_http_send_response_imp(NGX_HTTP_OK, response, r);
    }
    ngx_http_finalize_request(r, rc);
}

static ngx_int_t __post_upstream(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustdb_ha_sync_ctx_t * ctx = data;
    ngx_bool_t err = false;
    if (NGX_OK == rc && NGX_HTTP_OK != r->headers_out.status)
    {
        err = true;
    }
    ngx_str_t response = { ngx_http_get_buf_size(&r->upstream->buffer), r->upstream->buffer.pos };
    __finialize(err, &response, ctx->r);
    return NGX_OK;
}

ngx_int_t hustdb_ha_fetch_sync_data(
    const ngx_str_t * http_uri,
    const ngx_str_t * http_args,
    const ngx_str_t * user,
    const ngx_str_t * passwd,
    ngx_http_upstream_rr_peer_t * sync_peer,
    ngx_http_request_t * r)
{
    hustdb_ha_sync_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustdb_ha_sync_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(hustdb_ha_sync_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);
    ctx->r = r;

    static ngx_http_fetch_header_t headers[] = {
            { ngx_string("Connection"), ngx_string("Keep-Alive") },
            { ngx_string("Content-Type"), ngx_string("text/plain") }
    };
    static size_t headers_len = sizeof(headers) / sizeof(ngx_http_fetch_header_t);
    ngx_http_auth_basic_key_t auth = { *user, *passwd  };

    ngx_http_fetch_args_t args = {
        NGX_HTTP_GET,
        { sync_peer->sockaddr, sync_peer->socklen, &sync_peer->name, NULL },
        *http_uri,
        *http_args,
        { headers, headers_len },
        ngx_null_string,
        { NULL, NULL },
        { __post_upstream, ctx }
    };
    ngx_int_t rc = ngx_http_fetch(&args, &auth);
    if (NGX_OK != rc)
    {
        return NGX_ERROR;
    }
    ++r->main->count;
    r->write_event_handler = ngx_http_request_empty_handler;
    return NGX_OK;
}
