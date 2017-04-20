#ifndef __ngx_http_fetch_cache_20160104102346_h__
#define __ngx_http_fetch_cache_20160104102346_h__

#include "ngx_http_fetch_utils.h"

// keepalive cache
typedef struct
{
    ngx_queue_t queue;
    ngx_connection_t * connection;
    socklen_t socklen;
    ngx_sockaddr_t sockaddr;
} ngx_http_fetch_keepalive_cache_t;

ngx_int_t ngx_http_fetch_upstream_cache_init(
    size_t cache_size,
    ngx_pool_t * pool,
    ngx_queue_t * cache,
    ngx_queue_t * free);

ngx_connection_t * ngx_http_fetch_upstream_reuse_connection(
    ngx_peer_connection_t *pc,
    ngx_queue_t * cache,
    ngx_queue_t * free);

void ngx_http_fetch_upstream_reuse_cache(ngx_http_fetch_keepalive_cache_t * item, ngx_queue_t * free);

typedef void (*ngx_http_fetch_close_conn_t)(ngx_connection_t *c);

ngx_http_fetch_keepalive_cache_t * ngx_http_fetch_upstream_get_free_connection(
    ngx_http_fetch_close_conn_t close,
    ngx_queue_t * cache,
    ngx_queue_t * free);

// connection cache
typedef struct
{
    ngx_queue_t queue;
    ngx_connection_t connection;
    ngx_http_request_t request;
    ngx_pool_t * conn_pool;
    ngx_pool_t * request_pool;
} ngx_http_fetch_conn_cache_t;

ngx_int_t ngx_http_fetch_create_pool(
    size_t request_pool_size,
    ngx_log_t * log,
    ngx_http_fetch_conn_cache_t * item);

ngx_int_t ngx_http_fetch_conn_cache_init(
    size_t cache_size,
    ngx_pool_t * pool,
    ngx_queue_t * cache,
    ngx_queue_t * free);

ngx_int_t ngx_http_fetch_reuse_connection(
    ngx_connection_t * conn,
    ngx_queue_t * cache,
    ngx_queue_t * free);

ngx_int_t ngx_http_fetch_adjust_connection(ngx_connection_t * conn, ngx_queue_t * cache);

ngx_http_fetch_conn_cache_t * ngx_http_get_free_connection(ngx_queue_t * cache, ngx_queue_t * free);

#endif // __ngx_http_fetch_cache_20160104102346_h__
