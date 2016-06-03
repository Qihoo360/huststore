#include "ngx_http_fetch_keepalive.h"
#include "ngx_http_fetch_cache.h"
// reference: ngx_http_upstream_keepalive_module

typedef struct
{
    ngx_http_upstream_rr_peer_data_t rrp;
    ngx_http_request_t *r;
    ngx_http_fetch_addr_t addr;
    ngx_http_upstream_t * upstream;
    void * data;
} ngx_http_fetch_keepalive_peer_ctx_t;

static ngx_queue_t g_cache = { 0, 0 };
static ngx_queue_t g_free = { 0, 0 };

ngx_int_t ngx_http_fetch_keepalive_init(size_t keepalive, ngx_conf_t *cf)
{
    return ngx_http_fetch_upstream_cache_init(keepalive, cf->pool, &g_cache, &g_free);
}

static ngx_int_t __get_peer(ngx_peer_connection_t *pc, void *data)
{
    ngx_http_fetch_keepalive_peer_ctx_t * ctx = data;

    pc->sockaddr = ctx->addr.sockaddr;
    pc->socklen = ctx->addr.socklen;
    pc->name = ctx->addr.name;
    pc->log = ctx->r->connection->log;

    if (ctx->addr.peer)
    {
        ctx->rrp.current = ctx->addr.peer;
        ctx->addr.peer->conns++;
    }

    ngx_connection_t * c = ngx_http_fetch_upstream_reuse_connection(pc, &g_cache, &g_free);
    if (c)
    {
        c->idle = 0;
        c->sent = 0;
        c->log = pc->log;
        c->read->log = pc->log;
        c->write->log = pc->log;
        c->pool->log = pc->log;

        pc->connection = c;
        pc->cached = 1;
		
		return NGX_DONE;
    }
    return NGX_OK;
}

static void ngx_http_fetch_keepalive_dummy_handler(ngx_event_t *ev)
{
}

static void ngx_http_fetch_keepalive_close(ngx_connection_t *c)
{
    ngx_destroy_pool(c->pool);
    ngx_close_connection(c);
}

static void ngx_http_fetch_keepalive_close_handler(ngx_event_t *ev)
{
    ngx_connection_t *c = NULL;
    do
    {
        c = ev->data;

        if (c->close)
        {
            break;
        }

        char buf[1];
        int n = recv(c->fd, buf, 1, MSG_PEEK);

        if (n == -1 && ngx_socket_errno == NGX_EAGAIN)
        {
            ev->ready = 0;

            if (ngx_handle_read_event(c->read, 0) != NGX_OK)
            {
                break;
            }

            return;
        }
        return;
    } while (0);

    ngx_http_fetch_keepalive_cache_t * item = c->data;
    ngx_http_fetch_keepalive_close(c);
    ngx_http_fetch_upstream_reuse_cache(item, &g_free);
}

static void __free_peer(ngx_peer_connection_t *pc, void *data, ngx_uint_t state)
{
    ngx_http_fetch_keepalive_peer_ctx_t * ctx = data;
    ngx_http_fetch_keepalive_cache_t *item;

    ngx_http_upstream_t * u = ctx->upstream;
    ngx_connection_t * c = pc->connection;

    do
    {
        if ((state & NGX_PEER_FAILED) || !c
            || c->read->eof || c->read->error || c->read->timedout
            || c->write->error || c->write->timedout)
        {
            break;
        }
        if (!u->keepalive)
        {
            break;
        }
        if (ngx_terminate || ngx_exiting)
        {
            break;
        }
        if (NGX_OK != ngx_handle_read_event(c->read, 0))
        {
            break;
        }

        item = ngx_http_fetch_upstream_get_free_connection(ngx_http_fetch_keepalive_close, &g_cache, &g_free);

        item->connection = c;

        pc->connection = NULL;

        if (c->read->timer_set)
        {
            ngx_del_timer(c->read);
        }
        if (c->write->timer_set)
        {
            ngx_del_timer(c->write);
        }

        c->write->handler = ngx_http_fetch_keepalive_dummy_handler;
        c->read->handler = ngx_http_fetch_keepalive_close_handler;

        c->data = item;
        c->idle = 1;
        c->log = ngx_cycle->log;
        c->read->log = ngx_cycle->log;
        c->write->log = ngx_cycle->log;
        c->pool->log = ngx_cycle->log;

        item->socklen = pc->socklen;
        ngx_memcpy(&item->sockaddr, pc->sockaddr, pc->socklen);

        if (c->read->ready)
        {
            ngx_http_fetch_keepalive_close_handler(c->read);
        }
        return;
    } while (0);

    if (ctx->addr.peer)
    {
        ngx_http_upstream_free_round_robin_peer(pc, data, state);
    }
    else
    {
        pc->tries = 0;
    }
}

ngx_int_t ngx_http_fetch_init_keepalive_peer(
    ngx_http_fetch_addr_t * addr,
    ngx_http_request_t *r,
    ngx_http_upstream_srv_conf_t *us)
{
    ngx_http_fetch_keepalive_peer_ctx_t * ctx = ngx_palloc(r->pool, sizeof(ngx_http_fetch_keepalive_peer_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }

    memcpy(&ctx->addr, addr, sizeof(ngx_http_fetch_addr_t));
    ctx->r = r;
    ctx->upstream = r->upstream;
    ctx->data = r->upstream->peer.data;

    r->upstream->peer.data = ctx;

    if (addr->peer)
    {
        // r->upstream->peer.tries = ngx_http_upstream_tries(rrp->peers);
        if (NGX_OK != ngx_http_upstream_init_round_robin_peer(r, us))
        {
            return NGX_ERROR;
        }
    }
    else
    {
        r->upstream->peer.tries = 1;
    }

    r->upstream->peer.free = __free_peer;
    r->upstream->peer.get = __get_peer;

    return NGX_OK;
}
