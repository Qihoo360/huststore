#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_handler_filter.h"
#include "hustmq_ha_peer_def.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_int_t peer_count;
    ngx_int_t count;
} hustmq_ha_put_ctx_t;

static void __post_body_handler(ngx_http_request_t * r)
{
    hustmq_ha_put_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    --r->main->count;
    ngx_http_gen_subrequest(ctx->base.backend_uri, r, NULL,
        &ctx->base, ngx_http_post_subrequest_handler);
}

static ngx_int_t __first_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_str_t queue = hustmq_ha_get_queue(r);
    if (!queue.data)
    {
        return NGX_ERROR;
    }

    hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();
    if (!queue_dict)
    {
        return NGX_ERROR;
    }

	if (queue_dict->dict.ref && !hustmq_ha_put_queue_item_check(queue_dict, &queue))
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }
    size_t peer_count = ngx_http_get_backend_count();
    if (peer_count < 1)
    {
        return NGX_ERROR;
    }
    hustmq_ha_put_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_put_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(hustmq_ha_put_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);

    ctx->base.backend_uri = backend_uri;

    ctx->peer_count = peer_count;
    ctx->count = 0;

    ngx_int_t rc = ngx_http_read_client_request_body(r, __post_body_handler);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

static ngx_int_t __put_by_round_robin(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustmq_ha_put_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __first_put_handler(backend_uri, r);
    }
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        return (++ctx->count < ctx->peer_count) ? ngx_http_run_subrequest(r, &ctx->base, NULL) : NGX_ERROR;
    }
    return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
}


typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
    ngx_http_upstream_rr_peer_t * hash_peer;
} hustmq_ha_hash_put_ctx_t;

static ngx_http_upstream_rr_peer_t * __get_next_peer(
    ngx_http_upstream_rr_peer_t * hash_peer,
    ngx_http_upstream_rr_peer_t * peer)
{
    if (!peer)
    {
        return NULL;
    }
    do
    {
        peer = peer->next;
        if (!peer)
        {
            return NULL;
        }
        if (peer != hash_peer && ngx_http_peer_is_alive(peer))
        {
            return peer;
        }
    } while(peer->next);
    return NULL;
}

static ngx_http_upstream_rr_peer_t * __get_first_peer(ngx_http_upstream_rr_peer_t * hash_peer)
{
    ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
    if (!peers || !peers->peer)
    {
        return NULL;
    }
    ngx_http_upstream_rr_peer_t * peer = peers->peer;
    if (peer != hash_peer && ngx_http_peer_is_alive(peer))
    {
        return peer;
    }
    return __get_next_peer(hash_peer, peer);
}

static void __on_body_received(ngx_http_request_t * r)
{
    hustmq_ha_hash_put_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    --r->main->count;
    ngx_http_gen_subrequest(ctx->base.backend_uri, r, ctx->peer,
        &ctx->base, ngx_http_post_subrequest_handler);
}

static ngx_int_t __first_hash_put(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_str_t queue = hustmq_ha_get_queue(r);
    if (!queue.data)
    {
        return NGX_ERROR;
    }

    hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();
    if (!queue_dict)
    {
        return NGX_ERROR;
    }

    if (queue_dict->dict.ref && !hustmq_ha_put_queue_item_check(queue_dict, &queue))
    {
        return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
    }

    ngx_http_upstream_rr_peer_t * hash_peer = hustmq_ha_get_peer(&queue);
    ngx_bool_t alive = ngx_http_peer_is_alive(hash_peer);
    ngx_http_upstream_rr_peer_t * peer = alive ? hash_peer : __get_first_peer(hash_peer);

    hustmq_ha_hash_put_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_hash_put_ctx_t));
    if (!ctx)
    {
        return NGX_ERROR;
    }
    memset(ctx, 0, sizeof(hustmq_ha_hash_put_ctx_t));
    ngx_http_set_addon_module_ctx(r, ctx);
    ctx->base.backend_uri = backend_uri;
    ctx->hash_peer = hash_peer;
    ctx->peer = peer;

    ngx_int_t rc = ngx_http_read_client_request_body(r, __on_body_received);
    if ( rc >= NGX_HTTP_SPECIAL_RESPONSE )
    {
        return rc;
    }
    return NGX_DONE;
}

static ngx_int_t __put_by_queue_hash(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    hustmq_ha_hash_put_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        return __first_hash_put(backend_uri, r);
    }
    if (NGX_HTTP_OK != r->headers_out.status)
    {
        ctx->peer = __get_next_peer(ctx->hash_peer, ctx->peer);
        return ctx->peer ? ngx_http_run_subrequest(r, &ctx->base, ctx->peer) : NGX_ERROR;
    }
    return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
}

ngx_int_t hustmq_ha_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_hustmq_ha_main_conf_t * conf = hustmq_ha_get_module_main_conf(r);
    if (!conf)
    {
        return NGX_ERROR;
    }
    return conf->queue_hash ? __put_by_queue_hash(backend_uri, r) : __put_by_round_robin(backend_uri, r);
}
