#ifndef __ngx_http_fetch_encode_20151225175340_h__
#define __ngx_http_fetch_encode_20151225175340_h__

#include "ngx_http_fetch_utils.h"

ngx_int_t ngx_http_fetch_encode(
    const ngx_str_t * host,
    const ngx_http_auth_basic_key_t * auth,
    const ngx_http_fetch_headers_t * headers,
    const ngx_buf_t * body,
    ngx_http_request_t *r);

#endif // __ngx_http_fetch_encode_20151225175340_h__
