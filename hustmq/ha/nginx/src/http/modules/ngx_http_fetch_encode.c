#include "ngx_http_fetch_encode.h"

typedef struct
{
    ngx_http_fetch_header_t authorization;
    ngx_http_fetch_header_t host;
    ngx_http_fetch_header_t content_length;
} ngx_http_fetch_fixed_headers;

static const ngx_str_t AUTHORIZATION_KEY = ngx_string("Authorization");
static const ngx_str_t HOST_KEY = ngx_string("Host");
static const ngx_str_t CONTENT_LENGTH_KEY = ngx_string("Content-Length");
static const char PROXY_VERSION_11[] = " HTTP/1.1" CRLF;
static const char SPACE = ' ';
static const size_t SPACE_LEN = sizeof(SPACE);

static size_t __get_request_line_len(size_t method_len, size_t uri_len, size_t args_len)
{
    return (method_len + SPACE_LEN + sizeof(PROXY_VERSION_11) - 1 + sizeof(CRLF) - 1) + (
        uri_len + sizeof("?") - 1 + args_len);
}

static size_t __get_fixed_headers_len(const ngx_http_fetch_fixed_headers * headers)
{
    size_t len = 0;
    if (headers->authorization.value.data && headers->authorization.value.len > 0)
    {
        len += ngx_http_fetch_get_header_len(&headers->authorization);
    }
    len += ngx_http_fetch_get_header_len(&headers->host);
    len += ngx_http_fetch_get_header_len(&headers->content_length);
    return len;
}

static size_t __get_headlers_len(const ngx_http_fetch_headers_t * headers)
{
    size_t len = 0;
    size_t i = 0;
    for (i = 0; i < headers->size ; ++i)
    {
        len += ngx_http_fetch_get_header_len(&headers->arr[i]);
    }
    return len;
}

static void __encode_request_line(const ngx_str_t * method, ngx_http_request_t *r, ngx_buf_t * b)
{
    b->last = ngx_copy(b->last, method->data, method->len);
    b->last = ngx_copy(b->last, &SPACE, SPACE_LEN);
    b->last = ngx_copy(b->last, r->uri.data, r->uri.len);
    if (r->args.data && r->args.len > 0)
    {
        *b->last++ = '?';
        b->last = ngx_copy(b->last, r->args.data, r->args.len);
    }
    b->last = ngx_cpymem(b->last, PROXY_VERSION_11, sizeof(PROXY_VERSION_11) - 1);
}

static void __encode_fixed_headers(const ngx_http_fetch_fixed_headers * headers, ngx_buf_t * b)
{
    if (headers->authorization.value.data && headers->authorization.value.len > 0)
    {
        ngx_http_fetch_encode_header(&headers->authorization, b);
    }
    ngx_http_fetch_encode_header(&headers->host, b);
    ngx_http_fetch_encode_header(&headers->content_length, b);
}

static void __encode_headers(const ngx_http_fetch_headers_t * headers, ngx_buf_t * b)
{
    size_t i = 0;
    for (i = 0; i < headers->size ; ++i)
    {
        ngx_http_fetch_encode_header(&headers->arr[i], b);
    }
}

static void __encode_header_tail(ngx_buf_t * b)
{
    // add "\r\n" at the header end
    *b->last++ = CR; *b->last++ = LF;
}

static ngx_int_t __make_http_auth_basic(
    const ngx_str_t * user,
    const ngx_str_t * password,
    ngx_pool_t * pool,
    ngx_str_t * auth)
{
    static const ngx_str_t HEAD = ngx_string("Basic ");
    static const char SPLIT = ':';
    ngx_str_t src = { 0, 0 };
    src.len = user->len + password->len + 1;
    src.data = ngx_palloc(pool, src.len);
    if (!src.data)
    {
        return NGX_ERROR;
    }
    memcpy(src.data, user->data, user->len);
    memcpy(src.data + user->len, &SPLIT, 1);
    memcpy(src.data + user->len + 1, password->data, password->len);

    ngx_str_t dst = { 0, 0 };
    size_t len = ngx_base64_encoded_length(src.len);
    dst.data = ngx_palloc(pool, len);
    if (!dst.data)
    {
        return NGX_ERROR;
    }
    ngx_encode_base64(&dst, &src);

    auth->len = HEAD.len + dst.len;
    auth->data = ngx_palloc(pool, auth->len);
    if (!auth->data)
    {
        return NGX_ERROR;
    }

    memcpy(auth->data, HEAD.data, HEAD.len);
    memcpy(auth->data + HEAD.len, dst.data, dst.len);

    return NGX_OK;
}

static ngx_int_t __set_fixed_headers(
    const ngx_http_auth_basic_key_t * auth,
    const ngx_str_t * host,
    int content_length,
    ngx_http_request_t *r,
    ngx_http_fetch_fixed_headers * headers)
{
    if (auth && auth->user.data && auth->user.len > 0 && auth->password.data && auth->password.len > 0)
    {
        if (NGX_OK != __make_http_auth_basic(
            &auth->user, &auth->password, r->pool, &headers->authorization.value))
        {
            return NGX_ERROR;
        }
    }

    char content_length_str[20];
    sprintf(content_length_str, "%d", content_length);

    size_t len = strlen(content_length_str);
    headers->content_length.value.len = len;
    headers->content_length.value.data = ngx_palloc(r->pool, len);
    if (!headers->content_length.value.data)
    {
        return NGX_ERROR;
    }
    memcpy(headers->content_length.value.data, content_length_str, len);

    headers->host.value.len = host->len;
    headers->host.value.data = ngx_palloc(r->pool, host->len);
    if (!headers->host.value.data)
    {
        return NGX_ERROR;
    }
    memcpy(headers->host.value.data, host->data, host->len);

    return NGX_OK;
}

ngx_int_t ngx_http_fetch_encode(
    const ngx_str_t * host,
    const ngx_http_auth_basic_key_t * auth,
    const ngx_http_fetch_headers_t * headers,
    const ngx_buf_t * body,
    ngx_http_request_t *r)
{
    ngx_http_fetch_fixed_headers fixed_headers = {
        { AUTHORIZATION_KEY,  ngx_null_string },
        { HOST_KEY,           ngx_null_string },
        { CONTENT_LENGTH_KEY, ngx_null_string },
    };

    int content_length = body ? (int) (body->last - body->start) : 0;

    if (NGX_OK != __set_fixed_headers(auth, host, content_length, r, &fixed_headers))
    {
        return NGX_ERROR;
    }

    ngx_str_t method;
    method = r->method_name;

    size_t request_line_len = __get_request_line_len(method.len, r->uri.len, r->args.len);
    size_t fixed_headers_len = __get_fixed_headers_len(&fixed_headers);
    size_t headlers_len = __get_headlers_len(headers);

    size_t len = request_line_len + fixed_headers_len + headlers_len;

    ngx_buf_t * b = ngx_create_temp_buf(r->pool, len);
    if (!b)
    {
        return NGX_ERROR;
    }
    ngx_chain_t * cl = ngx_alloc_chain_link(r->pool);
    if (!cl)
    {
        return NGX_ERROR;
    }
    ngx_chain_t * request_bufs = cl;
    cl->buf = b;

    __encode_request_line(&method, r, b);
    __encode_fixed_headers(&fixed_headers, b);
    __encode_headers(headers, b);
    __encode_header_tail(b);

    // encode body
    if (content_length > 0)
    {
        b = ngx_alloc_buf(r->pool);
        if (!b)
        {
            return NGX_ERROR;
        }

        ngx_memcpy(b, body, sizeof(ngx_buf_t));

        cl->next = ngx_alloc_chain_link(r->pool);
        if (!cl->next)
        {
            return NGX_ERROR;
        }

        cl = cl->next;
        cl->buf = b;
    }

    b->flush = 1;
    cl->next = NULL;

    r->upstream->request_bufs = request_bufs;
    return NGX_OK;
}
