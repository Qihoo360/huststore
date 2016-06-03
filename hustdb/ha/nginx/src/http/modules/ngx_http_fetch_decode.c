#include "ngx_http_fetch_decode.h"

static ngx_int_t __decode_header_line(ngx_http_request_t *r, ngx_hash_t * headers_in_hash)
{
    ngx_table_elt_t * h = ngx_list_push(&r->upstream->headers_in.headers);
    if (!h)
    {
        return NGX_ERROR;
    }

    h->hash = r->header_hash;

    h->key.len = r->header_name_end - r->header_name_start;
    h->value.len = r->header_end - r->header_start;

    h->key.data = ngx_pnalloc(r->pool, h->key.len + 1 + h->value.len + 1 + h->key.len);
    if (h->key.data == NULL)
    {
        return NGX_ERROR;
    }

    h->value.data = h->key.data + h->key.len + 1;
    h->lowcase_key = h->key.data + h->key.len + 1 + h->value.len + 1;

    ngx_memcpy(h->key.data, r->header_name_start, h->key.len);
    h->key.data[h->key.len] = '\0';
    ngx_memcpy(h->value.data, r->header_start, h->value.len);
    h->value.data[h->value.len] = '\0';

    if (h->key.len == r->lowcase_index)
    {
        ngx_memcpy(h->lowcase_key, r->lowcase_header, h->key.len);
    }
    else
    {
        ngx_strlow(h->lowcase_key, h->key.data, h->key.len);
    }

    ngx_http_upstream_header_t * hh = ngx_hash_find(
        headers_in_hash, h->hash, h->lowcase_key, h->key.len);

    if (hh && hh->handler(r, h, hh->offset) != NGX_OK)
    {
        return NGX_ERROR;
    }
    return NGX_OK;
}

static ngx_int_t __finish_decode(ngx_http_request_t *r)
{
    if (!r->upstream->headers_in.server)
    {
        ngx_table_elt_t * h = ngx_list_push(&r->upstream->headers_in.headers);
        if (!h)
        {
            return NGX_ERROR;
        }

        h->hash = ngx_hash(ngx_hash(ngx_hash(ngx_hash(ngx_hash('s', 'e'), 'r'), 'v'), 'e'), 'r');

        ngx_str_set(&h->key, "Server");
        ngx_str_null(&h->value);
        h->lowcase_key = (u_char *) "server";
    }

    if (!r->upstream->headers_in.date)
    {
        ngx_table_elt_t * h = ngx_list_push(&r->upstream->headers_in.headers);
        if (!h)
        {
            return NGX_ERROR;
        }

        h->hash = ngx_hash(ngx_hash(ngx_hash('d', 'a'), 't'), 'e');

        ngx_str_set(&h->key, "Date");
        ngx_str_null(&h->value);
        h->lowcase_key = (u_char *) "date";
    }

    // clear content length if response is chunked
    ngx_http_upstream_t * u = r->upstream;

    if (u->headers_in.chunked)
    {
        u->headers_in.content_length_n = -1;
    }

    // set u->keepalive if response has no body; this allows to keep
    // connections alive in case of r->header_only or X-Accel-Redirect
    if (u->headers_in.status_n == NGX_HTTP_NO_CONTENT
        || u->headers_in.status_n == NGX_HTTP_NOT_MODIFIED
        || (!u->headers_in.chunked && u->headers_in.content_length_n == 0))
    {
        u->keepalive = !u->headers_in.connection_close;
    }

    if (u->headers_in.status_n == NGX_HTTP_SWITCHING_PROTOCOLS)
    {
        u->keepalive = 0;

        if (r->headers_in.upgrade)
        {
            u->upgrade = 1;
        }
    }

    return NGX_OK;
}

// reference: ngx_http_proxy_process_header
static ngx_int_t __decode_header(ngx_http_request_t *r)
{
    ngx_http_fetch_ctx_t * ctx = ngx_http_fetch_get_module_ctx(r);

    for (;;)
    {
        ngx_int_t rc = ngx_http_parse_header_line(r, &r->upstream->buffer, 1);

        if (rc == NGX_OK)
        {
            // a header line has been parsed successfully
            if (NGX_ERROR == __decode_header_line(r, ctx->headers_in_hash))
            {
                return NGX_ERROR;
            }
            continue;
        }

        if (rc == NGX_HTTP_PARSE_HEADER_DONE)
        {
            // a whole header has been parsed successfully
            return __finish_decode(r);
        }

        if (rc == NGX_AGAIN)
        {
            return NGX_AGAIN;
        }

        // there was error while a header line parsing
        return NGX_HTTP_UPSTREAM_INVALID_HEADER;
    }
    return NGX_ERROR;
}

// reference: ngx_http_proxy_process_status_line
ngx_int_t ngx_http_fetch_decode(ngx_http_request_t *r)
{
    ngx_http_fetch_ctx_t * ctx = ngx_http_fetch_get_module_ctx(r);
    if (!ctx)
    {
        return NGX_ERROR;
    }

    ngx_http_upstream_t * u = r->upstream;

    ngx_int_t rc = ngx_http_parse_status_line(r, &u->buffer, &ctx->status);

    if (rc == NGX_AGAIN)
    {
        return rc;
    }

    if (rc == NGX_ERROR)
    {
        r->http_version = NGX_HTTP_VERSION_9;
        u->state->status = NGX_HTTP_OK;
        u->headers_in.connection_close = 1;

        return NGX_OK;
    }

    if (u->state && u->state->status == 0)
    {
        u->state->status = ctx->status.code;
    }

    u->headers_in.status_n = ctx->status.code;

    size_t len = ctx->status.end - ctx->status.start;
    u->headers_in.status_line.len = len;

    u->headers_in.status_line.data = ngx_pnalloc(r->pool, len);
    if (!u->headers_in.status_line.data)
    {
        return NGX_ERROR;
    }

    ngx_memcpy(u->headers_in.status_line.data, ctx->status.start, len);

    if (ctx->status.http_version < NGX_HTTP_VERSION_11)
    {
        u->headers_in.connection_close = 1;
    }

    u->process_header = __decode_header;

    return __decode_header(r);
}
