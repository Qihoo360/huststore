#include "hustmq_ha_peer_def.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_handler_filter.h"

static ngx_http_peer_dict_t g_peer_dict;
static ngx_http_subrequest_peer_t * g_peer_list = NULL;

static ngx_bool_t __inconsistent(const hustmq_ha_idx_t * host_item, const hustmq_ha_idx_t * merge_item)
{
    if (!host_item || !merge_item)
    {
        return false;
    }
    return host_item->consistent_idx != merge_item->consistent_idx;
}

typedef struct
{
    ngx_http_subrequest_peer_t * node;
    hustmq_ha_idx_t * idx;
    ngx_pool_t * pool;
} build_pub_peer_list_ctx_t;

static ngx_bool_t __build_pub_peer_list(const char * host, const hustmq_ha_queue_item_t *item, void * data)
{
    if (!item || !data)
    {
        return false;
    }

    build_pub_peer_list_ctx_t * ctx = (build_pub_peer_list_ctx_t *)data;

    ngx_http_upstream_rr_peer_t * peer = ngx_http_peer_dict_get(&g_peer_dict, host);
    if (!peer)
    {
        return false;
    }
    ctx->node = ngx_http_append_peer(ctx->pool, peer, ctx->node);
    if (!ctx->node)
    {
        return false;
    }
    ngx_bool_t * inconsistent = ngx_palloc(ctx->pool, sizeof(ngx_bool_t));
    *inconsistent = __inconsistent(&item->base.idx, ctx->idx);
    ctx->node->data = inconsistent;
    return true;
}

ngx_http_subrequest_peer_t * hustmq_ha_build_pub_peer_list(hustmq_ha_queue_ctx_t * queue_ctx, ngx_pool_t * pool)
{
    if (!queue_ctx->queue_val)
    {
        return g_peer_list;
    }
    ngx_http_subrequest_peer_t * head = ngx_palloc(pool, sizeof(ngx_http_subrequest_peer_t));
    if (!head)
    {
        return NULL;
    }
    memset(head, 0, sizeof(ngx_http_subrequest_peer_t));

    build_pub_peer_list_ctx_t ctx = { head, &queue_ctx->item.idx, pool };

    if (!hustmq_ha_traverse_host_dict(queue_ctx->queue_val, __build_pub_peer_list, &ctx))
    {
        return NULL;
    }

    return head;
}

typedef struct
{
    ngx_http_subrequest_peer_t * node;
    int idx;
    ngx_pool_t * pool;
} build_sub_peer_list_ctx_t;

static ngx_bool_t __build_sub_peer_list(const char * host, const hustmq_ha_queue_item_t *item, void * data)
{
    if (!item || !data)
    {
        return false;
    }

    build_sub_peer_list_ctx_t * ctx = (build_sub_peer_list_ctx_t *)data;
    ngx_http_upstream_rr_peer_t * peer = ngx_http_peer_dict_get(&g_peer_dict, host);
    if (!peer)
    {
        return false;
    }
    if (hustmq_ha_idx_check(ctx->idx, item->base.idx.start_idx, item->base.idx.consistent_idx))
    {
        ctx->node = ngx_http_append_peer(ctx->pool, peer, ctx->node);
    }
    if (!ctx->node)
    {
        return false;
    }
    return true;
}

ngx_http_subrequest_peer_t * hustmq_ha_build_sub_peer_list(hustmq_ha_queue_value_t * queue_val, int idx, ngx_pool_t * pool)
{
    if (!queue_val)
    {
        return g_peer_list;
    }
    ngx_http_subrequest_peer_t * head = ngx_palloc(pool, sizeof(ngx_http_subrequest_peer_t));
    if (!head)
    {
        return NULL;
    }
    memset(head, 0, sizeof(ngx_http_subrequest_peer_t));

    build_sub_peer_list_ctx_t ctx = { head, idx, pool };

    if (!hustmq_ha_traverse_host_dict(queue_val, __build_sub_peer_list, &ctx))
    {
        return NULL;
    }

    return head;
}

ngx_bool_t hustmq_ha_inconsistent(ngx_http_subrequest_peer_t * peer)
{
    if (!peer || !peer->data)
    {
        return false;
    }
    return *(ngx_bool_t *)(peer->data);
}

ngx_bool_t hustmq_ha_init_peer_dict(ngx_http_upstream_rr_peers_t * peers)
{
    return ngx_http_init_peer_dict(peers, &g_peer_dict);
}

ngx_bool_t hustmq_ha_init_peer_list(ngx_pool_t * pool, ngx_http_upstream_rr_peers_t * peers)
{
    g_peer_list = ngx_http_init_peer_list(pool, peers);
    return !!g_peer_list;
}

