#include "ngx_http_fetch_upstream_handler.h"
#include "ngx_http_fetch_encode.h"
#include "ngx_http_fetch_decode.h"

// reference: ngx_http_proxy_input_filter_init
ngx_int_t ngx_http_fetch_upstream_input_filter_init(void *data)
{
    ngx_http_request_t * r = data;
    ngx_http_upstream_t * u = r->upstream;
    ngx_http_fetch_ctx_t * ctx = ngx_http_fetch_get_module_ctx(r);

    if (!ctx)
    {
        return NGX_ERROR;
    }

    if (u->headers_in.status_n == NGX_HTTP_NO_CONTENT
        || u->headers_in.status_n == NGX_HTTP_NOT_MODIFIED
        || u->headers_in.content_length_n == 0)
    {
        u->pipe->length = 0;
        u->length = 0;
        u->keepalive = !u->headers_in.connection_close;
    }
    else
    {
        u->pipe->length = u->headers_in.content_length_n;
        u->length = u->headers_in.content_length_n;
    }

    return NGX_OK;
}

// reference: ngx_http_proxy_copy_filter
ngx_int_t ngx_http_fetch_upstream_copy_filter(ngx_event_pipe_t *p, ngx_buf_t *buf)
{
    if (buf->pos == buf->last)
    {
        return NGX_OK;
    }

    ngx_chain_t * cl = ngx_chain_get_free_buf(p->pool, &p->free);
    if (!cl)
    {
        return NGX_ERROR;
    }

    ngx_buf_t * b = cl->buf;

    ngx_memcpy(b, buf, sizeof(ngx_buf_t));
    b->shadow = buf;
    b->tag = p->tag;
    b->last_shadow = 1;
    b->recycled = 1;
    buf->shadow = b;

    if (p->in)
    {
        *p->last_in = cl;
    }
    else
    {
        p->in = cl;
    }
    p->last_in = &cl->next;

    if (p->length == -1)
    {
        return NGX_OK;
    }

    p->length -= b->last - b->pos;

    if (p->length == 0)
    {
        ngx_http_request_t * r = p->input_ctx;
        p->upstream_done = 1;
        r->upstream->keepalive = !r->upstream->headers_in.connection_close;

    }
    else if (p->length < 0)
    {
        p->upstream_done = 1;
    }
    return NGX_OK;
}

// reference: ngx_http_proxy_non_buffered_copy_filter
ngx_int_t ngx_http_fetch_upstream_non_buffered_copy_filter(void *data, ssize_t bytes)
{
    ngx_http_request_t *r = data;
    ngx_http_upstream_t * u = r->upstream;
    ngx_chain_t *cl = u->out_bufs;
    ngx_chain_t **ll = &u->out_bufs;
    while (cl)
    {
        ll = &cl->next;
        cl = cl->next;
    }

    cl = ngx_chain_get_free_buf(r->pool, &u->free_bufs);
    if (!cl)
    {
        return NGX_ERROR;
    }

    *ll = cl;

    cl->buf->flush = 1;
    cl->buf->memory = 1;

    ngx_buf_t * b = &u->buffer;

    cl->buf->pos = b->last;
    b->last += bytes;
    cl->buf->last = b->last;
    cl->buf->tag = u->output.tag;

    if (u->length == -1)
    {
        return NGX_OK;
    }

    u->length -= bytes;

    if (u->length == 0)
    {
        u->keepalive = !u->headers_in.connection_close;
    }

    return NGX_OK;
}

ngx_int_t ngx_http_fetch_upstream_create_request(ngx_http_request_t *r)
{
    ngx_http_fetch_ctx_t *ctx;
    ctx = ngx_http_fetch_get_module_ctx(r);
    if (NGX_OK != ngx_http_fetch_encode(ctx->addr.name, ctx->auth, &ctx->headers, ctx->body, r))
    {
        return NGX_ERROR;
    }
    return NGX_OK;
}

ngx_int_t ngx_http_fetch_upstream_process_header(ngx_http_request_t *r)
{
    return ngx_http_fetch_decode(r);
}

ngx_int_t ngx_http_fetch_upstream_reinit_request(ngx_http_request_t *r)
{
    ngx_http_fetch_ctx_t * ctx = ngx_http_fetch_get_module_ctx(r);
    ctx->status.code = 0;
    ctx->status.count = 0;
    ctx->status.start = NULL;
    ctx->status.end = NULL;

    r->upstream->process_header = ngx_http_fetch_decode;
    r->upstream->pipe->input_filter = ngx_http_fetch_upstream_copy_filter;
    r->upstream->input_filter = ngx_http_fetch_upstream_non_buffered_copy_filter;
    r->state = 0;
    return NGX_OK;
}

void ngx_http_fetch_upstream_abort_request(ngx_http_request_t *r)
{
    ngx_http_fetch_log("ngx_http_fetch::ngx_http_fetch_upstream_abort_request");
    return;
}

void ngx_http_fetch_upstream_finalize_request(ngx_http_request_t * r, ngx_int_t rc)
{
    ngx_http_fetch_ctx_t * ctx;
    ctx = ngx_http_fetch_get_module_ctx(r);
    if (ctx->post_upstream.handler)
    {
        ctx->post_upstream.handler(r, ctx->post_upstream.data, rc);
    }

    if (NGX_OK == rc)
    {
        // in normal case, nginx will call ngx_http_free_request & ngx_http_close_connection
        // to free the pool, so we need to increase the r->count to skip it
        ++r->count;
    }
    else
    {
        // in this case, nginx will call ngx_http_terminate_handler to free pool
        // so we need to adjust the pool
        ngx_http_fetch_log("ngx_http_fetch::ngx_http_fetch_upstream_finalize_request::adjust pool");
        // reference: ngx_http_fetch_upstream.c::__adjust_connection
        if (NGX_OK != ctx->adjust(r->connection))
        {
            ngx_http_fetch_log("ngx_http_fetch::ngx_http_fetch_upstream_finalize_request::adjust fail");
        }
    }

    // reference: ngx_http_fetch_upstream.c::__reuse_connection
    if (NGX_OK != ctx->reuse(r->connection))
    {
        ngx_http_fetch_log("ngx_http_fetch::ngx_http_fetch_upstream_finalize_request::reuse fail");
    }
    return;
}
