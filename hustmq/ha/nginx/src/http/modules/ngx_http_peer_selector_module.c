#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_addon_def.h>

typedef struct
{
    ngx_http_upstream_rr_peer_data_t   rrp;
    ngx_http_upstream_rr_peer_t * peer;
} ngx_http_peer_selector_peer_data_t;

static ngx_int_t __init_peer(ngx_http_request_t * r, ngx_http_upstream_srv_conf_t * us);
static ngx_int_t __get_peer(ngx_peer_connection_t * pc, void * data);
static char * ngx_http_peer_selector(ngx_conf_t * cf, ngx_command_t * cmd, void * conf);

static ngx_command_t  ngx_http_peer_selector_commands[] = {
    {
        ngx_string("customized_selector"),
        NGX_HTTP_UPS_CONF|NGX_CONF_NOARGS,
		ngx_http_peer_selector,
        0,
        0,
        NULL
    },

    ngx_null_command
};

static ngx_http_module_t  ngx_http_peer_selector_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
	NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */
    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

ngx_module_t  ngx_http_peer_selector_module = {
    NGX_MODULE_V1,
    &ngx_http_peer_selector_module_ctx,    /* module context */
    ngx_http_peer_selector_commands,       /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_peer_selector_init(ngx_conf_t * cf, ngx_http_upstream_srv_conf_t * us)
{
    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK)
    {
        return NGX_ERROR;
    }
    us->peer.init = __init_peer;
    return NGX_OK;
}

static ngx_int_t __init_peer(ngx_http_request_t * r, ngx_http_upstream_srv_conf_t * us)
{
    if (!r || !r->parent)
    {
        return NGX_ERROR;
    }

    ngx_http_peer_selector_peer_data_t  *peer_data = ngx_palloc(r->pool, sizeof(ngx_http_peer_selector_peer_data_t));
    if (!peer_data)
    {
        return NGX_ERROR;
    }
    peer_data->peer = ngx_http_get_addon_module_ctx(r);
    r->upstream->peer.data = &peer_data->rrp;
    if (ngx_http_upstream_init_round_robin_peer(r, us) != NGX_OK)
    {
        return NGX_ERROR;
    }

    r->upstream->peer.get = __get_peer;

    return NGX_OK;
}

static ngx_int_t __get_peer(ngx_peer_connection_t * pc, void * data)
{
    ngx_http_peer_selector_peer_data_t  *peer_data = data;
    if (!peer_data || !peer_data->peer)
    {
    	return ngx_http_upstream_get_round_robin_peer(pc, data);
    }

    peer_data->rrp.current = peer_data->peer;

	pc->sockaddr = peer_data->peer->sockaddr;
	pc->socklen = peer_data->peer->socklen;
	pc->name = &peer_data->peer->name;

	peer_data->peer->conns++;

    return NGX_OK;
}

static char * ngx_http_peer_selector(ngx_conf_t * cf, ngx_command_t * cmd, void * conf)
{
    ngx_http_upstream_srv_conf_t  *uscf;

    uscf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_upstream_module);

    if (uscf->peer.init_upstream)
    {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0, "load balancing method redefined");
    }

    uscf->peer.init_upstream = ngx_http_peer_selector_init;

    uscf->flags = NGX_HTTP_UPSTREAM_CREATE
                  |NGX_HTTP_UPSTREAM_WEIGHT
                  |NGX_HTTP_UPSTREAM_MAX_FAILS
                  |NGX_HTTP_UPSTREAM_FAIL_TIMEOUT
                  |NGX_HTTP_UPSTREAM_DOWN;

    return NGX_CONF_OK;
}

