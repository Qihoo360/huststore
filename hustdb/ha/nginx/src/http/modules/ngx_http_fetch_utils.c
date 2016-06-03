#include "ngx_http_fetch_utils.h"

static ngx_str_t HTTP_GET = ngx_string("GET");
static ngx_str_t HTTP_POST = ngx_string("POST");
static ngx_str_t HTTP_PUT = ngx_string("PUT");
static ngx_str_t HTTP_DELETE = ngx_string("DELETE");

ngx_str_t ngx_http_fetch_get_method(int method)
{
    if (NGX_HTTP_POST == method)
    {
        return HTTP_POST;
    }
    if (NGX_HTTP_PUT == method)
    {
        return HTTP_PUT;
    }
    if (NGX_HTTP_DELETE == method)
    {
        return HTTP_DELETE;
    }
    return HTTP_GET;
}

size_t ngx_http_fetch_get_header_len(const ngx_http_fetch_header_t * header)
{
    return header->key.len + sizeof(": ") - 1
        + header->value.len + sizeof(CRLF) - 1;
}

void ngx_http_fetch_encode_header(const ngx_http_fetch_header_t * header, ngx_buf_t * b)
{
    b->last = ngx_copy(b->last, header->key.data, header->key.len);
    *b->last++ = ':'; *b->last++ = ' ';
    b->last = ngx_copy(b->last, header->value.data, header->value.len);
    *b->last++ = CR; *b->last++ = LF;
}

ngx_buf_t * ngx_http_fetch_create_buf(const ngx_str_t * data, ngx_pool_t * pool)
{
    ngx_buf_t* buf = ngx_create_temp_buf(pool, data->len);
    if (!buf)
    {
        return NULL;
    }
    buf->last = buf->pos + data->len;
    memcpy(buf->pos, data->data, data->len);
    return buf;
}

ngx_int_t ngx_http_fetch_copy_str(const ngx_str_t * src, ngx_pool_t * pool, ngx_str_t * dst)
{
    if (!src || !src->data || src->len < 1 || !pool || !dst)
    {
        return NGX_ERROR;
    }
    dst->data = ngx_palloc(pool, src->len);
    if (!dst->data)
    {
        return NGX_ERROR;
    }
    memcpy(dst->data, src->data, src->len);
    dst->len = src->len;
    return NGX_OK;
}

void ngx_http_fetch_log(const char * log)
{
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, log);
}
