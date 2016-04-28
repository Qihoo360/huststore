#include "ngx_http_fetch_cache.h"
#include <ngx_core.h>

ngx_int_t ngx_http_fetch_upstream_cache_init(
    size_t cache_size,
    ngx_pool_t * pool,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    ngx_http_fetch_keepalive_cache_t * cached = ngx_pcalloc(pool, sizeof(ngx_http_fetch_keepalive_cache_t) * cache_size);
    if (!cached)
    {
        return NGX_ERROR;
    }
    memset(cached, 0, sizeof(ngx_http_fetch_keepalive_cache_t) * cache_size);

    ngx_queue_init(cache);
    ngx_queue_init(free);

    size_t i = 0;
    for (i = 0; i < cache_size; i++)
    {
        ngx_queue_insert_head(free, &cached[i].queue);
    }
    return NGX_OK;
}

ngx_connection_t * ngx_http_fetch_upstream_reuse_connection(
    ngx_peer_connection_t *pc,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    ngx_queue_t * q = NULL;
    for (q = ngx_queue_head(cache); q != ngx_queue_sentinel(cache); q = ngx_queue_next(q))
    {
        ngx_http_fetch_keepalive_cache_t * item = ngx_queue_data(q, ngx_http_fetch_keepalive_cache_t, queue);
        ngx_connection_t * c = item->connection;

        if (0 == ngx_memn2cmp((u_char *) &item->sockaddr, (u_char *) pc->sockaddr, item->socklen, pc->socklen))
        {
            ngx_queue_remove(q);
            ngx_queue_insert_head(free, q);
            return c;
        }
    }
    return NULL;
}

ngx_http_fetch_keepalive_cache_t * ngx_http_fetch_upstream_get_free_connection(
    ngx_http_fetch_close_conn_t close,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    ngx_http_fetch_keepalive_cache_t * item = NULL;
    ngx_queue_t * q = NULL;
    if (ngx_queue_empty(free))
    {
        q = ngx_queue_last(cache);
        ngx_queue_remove(q);

        item = ngx_queue_data(q, ngx_http_fetch_keepalive_cache_t, queue);

        close(item->connection);
    }
    else
    {
        q = ngx_queue_head(free);
        ngx_queue_remove(q);

        item = ngx_queue_data(q, ngx_http_fetch_keepalive_cache_t, queue);
    }

    ngx_queue_insert_head(cache, q);

    return item;
}

void ngx_http_fetch_upstream_reuse_cache(ngx_http_fetch_keepalive_cache_t * item, ngx_queue_t * free)
{
    ngx_queue_remove(&item->queue);
    ngx_queue_insert_head(free, &item->queue);
}

static void __destory_pool(ngx_http_fetch_conn_cache_t * item)
{
    if (item->request_pool)
    {
        ngx_destroy_pool(item->request_pool);
        item->request_pool = NULL;
    }
    if (item->conn_pool)
    {
        ngx_destroy_pool(item->conn_pool);
        item->conn_pool = NULL;
    }
}

ngx_int_t ngx_http_fetch_create_pool(
    size_t request_pool_size,
    ngx_log_t * log,
    ngx_http_fetch_conn_cache_t * item)
{
    __destory_pool(item);

    do
    {
        item->request_pool = ngx_create_pool(request_pool_size, log);
        if (!item->request_pool)
        {
            break;
        }
        item->conn_pool = ngx_create_pool(ngx_pagesize, log);
        if (!item->conn_pool)
        {
            break;
        }
        return NGX_OK;
    } while (0);

    __destory_pool(item);

    return NGX_ERROR;
}

ngx_int_t ngx_http_fetch_conn_cache_init(
    size_t cache_size,
    ngx_pool_t * pool,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    ngx_http_fetch_conn_cache_t * cached = ngx_pcalloc(pool, sizeof(ngx_http_fetch_conn_cache_t) * cache_size);
    if (!cached)
    {
        return NGX_ERROR;
    }
    memset(cached, 0, sizeof(ngx_http_fetch_conn_cache_t) * cache_size);

    ngx_queue_init(cache);
    ngx_queue_init(free);

    size_t i = 0;
    for (i = 0; i < cache_size; i++)
    {
        ngx_queue_insert_head(free, &cached[i].queue);
    }
    return NGX_OK;
}

ngx_queue_t * __match_connection(ngx_queue_t * cache, ngx_connection_t * conn)
{
    ngx_queue_t * q = NULL;
    for (q = ngx_queue_head(cache); q != ngx_queue_sentinel(cache); q = ngx_queue_next(q))
    {
        ngx_http_fetch_conn_cache_t * item = ngx_queue_data(q, ngx_http_fetch_conn_cache_t, queue);
        ngx_connection_t * c = &item->connection;

        if (c == conn)
        {
            return q;
        }
    }
    return NULL;
}

ngx_int_t ngx_http_fetch_reuse_connection(
    ngx_connection_t * conn,
    ngx_queue_t * cache,
    ngx_queue_t * free)
{
    ngx_queue_t * q = __match_connection(cache, conn);
    if (!q)
    {
        return NGX_ERROR;
    }
    ngx_queue_remove(q);
    ngx_queue_insert_head(free, q);
    return NGX_OK;
}

ngx_int_t ngx_http_fetch_adjust_connection(ngx_connection_t * conn, ngx_queue_t * cache)
{
    ngx_queue_t * q = __match_connection(cache, conn);
    if (!q)
    {
        return NGX_ERROR;
    }
    ngx_http_fetch_conn_cache_t * item = ngx_queue_data(q, ngx_http_fetch_conn_cache_t, queue);
    // nginx will call ngx_http_terminate_handler to free pool
    // so we need to reset the pool to avoid double free
    item->conn_pool = NULL;
    item->request_pool = NULL;
    return NGX_OK;
}

ngx_http_fetch_conn_cache_t * ngx_http_get_free_connection(ngx_queue_t * cache, ngx_queue_t * free)
{
    if (ngx_queue_empty(free))
    {
        return NULL;
    }

    ngx_queue_t * q = ngx_queue_head(free);
    ngx_queue_remove(q);
    ngx_http_fetch_conn_cache_t * item = ngx_queue_data(q, ngx_http_fetch_conn_cache_t, queue);

    ngx_queue_insert_head(cache, q);

    return item;
}
