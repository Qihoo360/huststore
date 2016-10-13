#include "hustdb_ha_utils_inner.h"

static ngx_http_peer_array_t g_peer_array;
static hustdb_ha_peers_t g_peers;

ngx_bool_t hustdb_ha_init_peer_array(ngx_pool_t * pool)
{
    ngx_bool_t rc = ngx_http_init_peer_array(pool, ngx_http_get_backends(), ngx_http_get_backend_count(), &g_peer_array);
    if (!rc)
    {
        return false;
    }
    g_peers.size = g_peer_array.size;
    g_peers.peers = ngx_palloc(pool, g_peers.size * sizeof(const char *));
    size_t i = 0;
    for (i = 0; i < g_peers.size; ++i)
    {
        g_peers.peers[i] = (const char *)g_peer_array.arr[i]->server.data;
    }
    return true;
}

ngx_http_upstream_rr_peer_t * hustdb_ha_get_peer(ngx_http_request_t *r)
{
    char * val = ngx_http_get_param_val(&r->args, PEER_KEY, r->pool);
    if (!val)
    {
        return NULL;
    }
    size_t index = atoi(val);
    return ngx_http_get_peer_by_index(&g_peer_array, index);
}

size_t hustdb_ha_get_peer_array_count()
{
    return g_peer_array.size;
}

const char * hustdb_ha_get_peer_item_uri(size_t index)
{
    return (const char *) g_peer_array.arr[index]->server.data;
}
