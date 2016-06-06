#include "ngx_http_fetch.h"
#include "ngx_http_fetch_upstream.h"
#include "ngx_http_fetch_upstream_handler.h"
#include "ngx_http_fetch_keepalive.h"
#include <nginx.h>

typedef struct
{
    ngx_http_upstream_conf_t umcf;
    ngx_str_t schema;
    size_t request_pool_size;
    ngx_log_t * log;
    ngx_cycle_t * cycle;
    ngx_http_core_srv_conf_t * cscf;
    ngx_uint_t variables;
    ngx_hash_t headers_in_hash;
} ngx_http_fetch_conf_t;

static ngx_http_fetch_conf_t * g_conf = NULL;

// ngx_http_upstream.c:ngx_http_upstream_server
static ngx_http_upstream_server_t * ngx_http_parse_upstream_server(const ngx_str_t * url, ngx_conf_t * cf)
{
    ngx_int_t weight = 1;
    ngx_int_t max_fails = 1;
    time_t fail_timeout = 10;

    ngx_url_t u;
    ngx_memzero(&u, sizeof(ngx_url_t));
    u.url = *url;
    u.default_port = 80;

    if (NGX_OK != ngx_parse_url(cf->pool, &u))
    {
        return NULL;
    }

    ngx_http_upstream_server_t * us = ngx_palloc(cf->pool, sizeof(ngx_http_upstream_server_t));
    ngx_memzero(us, sizeof(ngx_http_upstream_server_t));

    us->name = u.url;
    us->addrs = u.addrs;
    us->naddrs = u.naddrs;
    us->weight = weight;
    us->max_fails = max_fails;
    us->fail_timeout = fail_timeout;

    return us;
}

// ngx_http_upstream_round_robin.c:ngx_http_upstream_init_round_robin
ngx_http_upstream_rr_peers_t  * ngx_http_init_upstream_rr_peers(const ngx_url_array_t * urls, ngx_conf_t * cf)
{
    if (!urls || !urls->arr || urls->size < 1 || !cf)
    {
        return NULL;
    }
    ngx_http_upstream_server_t ** servers = ngx_palloc(cf->pool, urls->size * sizeof(ngx_http_upstream_server_t *));
    size_t i = 0;
    for (i = 0; i < urls->size; ++i)
    {
        ngx_http_upstream_server_t * server = ngx_http_parse_upstream_server(urls->arr + i, cf);
        if (!server)
        {
            return NULL;
        }
        servers[i] = server;
    }
    ngx_uint_t w = 0;
    for (i = 0; i < urls->size; ++i)
    {
        w += servers[i]->naddrs * servers[i]->weight;
    }
    ngx_http_upstream_rr_peers_t * peers = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peers_t));
    if (!peers)
    {
        return NULL;
    }
    ngx_http_upstream_rr_peer_t * peer = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_rr_peer_t) * urls->size);
    if (!peer)
    {
        return NULL;
    }

    peers->single = (urls->size == 1);
    peers->number = urls->size;
    peers->weighted = (w != urls->size);
    peers->total_weight = w;

    ngx_http_upstream_rr_peer_t ** peerp = &peers->peer;
    for (i = 0; i < urls->size; ++i)
    {
        peer[i].sockaddr = servers[i]->addrs[0].sockaddr;
        peer[i].socklen = servers[i]->addrs[0].socklen;
        peer[i].name = servers[i]->addrs[0].name;
        peer[i].weight = servers[i]->weight;
        peer[i].effective_weight = servers[i]->weight;
        peer[i].current_weight = 0;
        peer[i].max_fails = servers[i]->max_fails;
        peer[i].fail_timeout = servers[i]->fail_timeout;
        peer[i].down = servers[i]->down;
        peer[i].server = servers[i]->name;
        *peerp = &peer[i];
        peerp = &peer[i].next;
    }
    return peers;
}

static void __init_upstream_conf(const ngx_http_fetch_upstream_conf_t * src, ngx_http_upstream_conf_t * dst)
{
    dst->connect_timeout = (src && 0 != src->connect_timeout) ? src->connect_timeout : 60000;
    dst->send_timeout = (src && 0 != src->send_timeout) ? src->send_timeout : 60000;
    dst->read_timeout = (src && 0 != src->read_timeout) ? src->read_timeout : 60000;
    dst->timeout = (src && 0 != src->timeout) ? src->timeout : 60000;

    dst->buffering = 1;
    dst->buffer_size = (src && 0 != src->buffer_size) ? src->buffer_size : ngx_pagesize;
    dst->bufs.num = (src && 0 != src->bufs.num) ? src->bufs.num : 8;
    dst->bufs.size = (src && 0 != src->bufs.size) ? src->bufs.size : dst->buffer_size;
    dst->busy_buffers_size = (src && 0 != src->busy_buffers_size) ? src->busy_buffers_size : 2 * dst->buffer_size;
}

static ngx_int_t __init_peer(ngx_http_request_t *r, ngx_http_upstream_srv_conf_t *us)
{
    ngx_http_fetch_ctx_t * ctx = ngx_http_fetch_get_module_ctx(r);
    return ngx_http_fetch_init_keepalive_peer(&ctx->addr, r, us);
}

static ngx_http_conf_port_t * __select_port(ngx_http_conf_port_t * arr, ngx_uint_t size)
{
    return arr;
}

static ngx_http_conf_addr_t * __select_addr(ngx_http_conf_addr_t * arr, ngx_uint_t size)
{
    return arr;
}

static ngx_http_core_srv_conf_t * __get_core_srv_conf(ngx_http_core_main_conf_t * cmcf, ngx_http_fetch_selector_t * selector)
{
    ngx_http_fetch_select_port_t select_port = (selector && selector->select_port
        ) ? selector->select_port : __select_port;
    ngx_http_fetch_select_addr_t select_addr = (selector && selector->select_addr
        ) ? selector->select_addr : __select_addr;

    ngx_http_conf_port_t * port = select_port((ngx_http_conf_port_t *)cmcf->ports->elts, cmcf->ports->nelts);
    if (!port)
    {
        return NULL;
    }
    ngx_http_conf_addr_t * addr = select_addr((ngx_http_conf_addr_t *)port->addrs.elts, port->addrs.nelts);
    if (!addr)
    {
        return NULL;
    }
    return addr->default_server;
}

ngx_int_t ngx_http_fetch_init_conf(
    const ngx_http_fetch_essential_conf_t * essential_conf,
    ngx_http_fetch_upstream_conf_t * uscf,
    ngx_http_fetch_selector_t * selector)
{
    if (!essential_conf || !essential_conf->cf)
    {
        return NGX_ERROR;
    }
    ngx_conf_t * cf = essential_conf->cf;
    g_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_fetch_conf_t));
    ngx_http_fetch_conf_t * conf = g_conf;

    conf->cycle = cf->cycle;
    conf->request_pool_size = essential_conf->request_pool_size;
    conf->log = cf->log;
    conf->schema.len = sizeof("nginx://") - 1;
    conf->schema.data = (u_char *) "nginx://";

    ngx_http_upstream_main_conf_t * umcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_upstream_module);
    if (!umcf)
    {
        return NGX_ERROR;
    }
    conf->headers_in_hash = umcf->headers_in_hash;

    ngx_http_core_main_conf_t * cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    if (!cmcf)
    {
        return NGX_ERROR;
    }
    conf->variables = cmcf->variables.nelts;
    conf->cscf = __get_core_srv_conf(cmcf, selector);

    if (!conf->cscf || !conf->cscf->ctx
        || !conf->cscf->ctx->main_conf
        || !conf->cscf->ctx->srv_conf
        || !conf->cscf->ctx->loc_conf)
    {
        return NGX_ERROR;
    }

    conf->umcf.upstream = ngx_pcalloc(cf->pool, sizeof(ngx_http_upstream_srv_conf_t));
    if (!conf->umcf.upstream || !essential_conf->peers)
    {
        return NGX_ERROR;
    }
    conf->umcf.upstream->peer.data = essential_conf->peers; // ngx_http_upstream_init_round_robin_peer

    ngx_http_upstream_conf_t * uc = &conf->umcf;
    uc->upstream->peer.init = __init_peer;
    __init_upstream_conf(uscf, uc);

    // reference: ngx_http_upstream_init_main_conf
    ngx_hash_init_t hash;
    hash.hash = &uc->hide_headers_hash;
    hash.key = ngx_hash_key_lc;
    hash.max_size = 512;
    hash.bucket_size = ngx_align(64, ngx_cacheline_size);
    hash.name = "fetch_hide_headers_hash";
    hash.pool = cf->pool;
    hash.temp_pool = NULL;

    ngx_int_t rc = ngx_hash_init(&hash, NULL, 0);
    if (NGX_OK != rc)
    {
        return NGX_ERROR;
    }
    if (NGX_OK != ngx_http_fetch_upstream_init(essential_conf->connection_cache_size, cf))
    {
        return NGX_ERROR;
    }
    return ngx_http_fetch_keepalive_init(essential_conf->keepalive_cache_size, cf);
}

static ngx_int_t __init_request(const ngx_http_fetch_args_t * args, ngx_http_request_t *r)
{
    r->method = args->http_method;
    r->http_version = NGX_HTTP_VERSION_11;
    // r->request_line;

    if (NGX_OK != ngx_http_fetch_copy_str(&args->uri, r->pool, &r->uri))
    {
        return NGX_ERROR;
    }
    if (args->args.data && args->args.len > 0)
    {
        if (NGX_OK != ngx_http_fetch_copy_str(&args->args, r->pool, &r->args))
        {
            return NGX_ERROR;
        }
    }
    else
    {
        r->args.data = 0;
        r->args.len = 0;
    }

    r->header_hash = 1;
    r->subrequest_in_memory = 1;
    // r->keepalive = !!args->keepalive;

    r->method_name = ngx_http_fetch_get_method(args->http_method);

    // r->http_protocol
    ngx_http_set_exten(r);
    ngx_time_t *tp = ngx_timeofday();
    r->start_sec = tp->sec;
    r->start_msec = tp->msec;

    return NGX_OK;
}

ngx_int_t ngx_http_fetch(const ngx_http_fetch_args_t * args, const ngx_http_auth_basic_key_t * auth)
{
    if (!args || !g_conf)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    ngx_http_request_t * r = ngx_http_fetch_create_request(
        g_conf->request_pool_size, g_conf->log, g_conf->variables, g_conf->cscf->ctx, g_conf->cycle);
    if (!r)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    if (NGX_OK != __init_request(args, r))
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    if (args->before_upstream.handler)
    {
        args->before_upstream.handler(r, args->before_upstream.data);
    }
    ngx_http_fetch_upstream_data_t upstream_data = {
        args,
        auth,
        &g_conf->headers_in_hash,
        &g_conf->schema,
        &g_conf->umcf,
        r
    };
    return ngx_http_fetch_init_upstream(&upstream_data);
}
