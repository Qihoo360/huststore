#ifndef __ngx_http_fetch_upstream_handler_20151230201744_h__
#define __ngx_http_fetch_upstream_handler_20151230201744_h__

#include "ngx_http_fetch_utils.h"

ngx_int_t ngx_http_fetch_upstream_input_filter_init(void *data);
ngx_int_t ngx_http_fetch_upstream_copy_filter(ngx_event_pipe_t *p, ngx_buf_t *buf);
ngx_int_t ngx_http_fetch_upstream_non_buffered_copy_filter(void *data, ssize_t bytes);

ngx_int_t ngx_http_fetch_upstream_create_request(ngx_http_request_t *r);
ngx_int_t ngx_http_fetch_upstream_process_header(ngx_http_request_t *r);
ngx_int_t ngx_http_fetch_upstream_reinit_request(ngx_http_request_t *r);
void ngx_http_fetch_upstream_abort_request(ngx_http_request_t *r);
void ngx_http_fetch_upstream_finalize_request(ngx_http_request_t *r, ngx_int_t rc);

#endif // __ngx_http_fetch_upstream_handler_20151230201744_h__
