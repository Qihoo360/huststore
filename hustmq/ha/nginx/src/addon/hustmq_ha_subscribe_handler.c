#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_peer_def.h"
#include "ngx_http_upstream_check_module.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
	ngx_http_subrequest_peer_t * subrequest_peer;
} hustmq_ha_sub_ctx_t;

static ngx_int_t __first_sub_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	ngx_str_t queue = hustmq_ha_get_queue(r);
	if (!queue.data)
	{
		return NGX_ERROR;
	}
    int idx = hustmq_ha_get_idx(r);
    if (idx < 0)
    {
        return NGX_ERROR;
    }

	hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();
	if (!queue_dict)
	{
		return NGX_ERROR;
	}

	hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(queue_dict, (const char *)queue.data);
	if (!queue_val)
	{
	    return NGX_ERROR;
	}
	ngx_http_subrequest_peer_t * peer = hustmq_ha_build_sub_peer_list(queue_val, idx, r->pool);
	if (!peer)
	{
	    return NGX_ERROR;
	}

	peer = ngx_http_get_first_peer(peer);
	if (!peer)
	{
	    return NGX_ERROR;
	}

	hustmq_ha_sub_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_sub_ctx_t));
	if (!ctx)
	{
		return NGX_ERROR;
	}
	memset(ctx, 0, sizeof(hustmq_ha_sub_ctx_t));
	ngx_http_set_addon_module_ctx(r, ctx);
	ctx->subrequest_peer = peer;

	return ngx_http_gen_subrequest(backend_uri, r, ctx->subrequest_peer->peer,
	        &ctx->base, ngx_http_post_subrequest_handler);
}

ngx_int_t hustmq_ha_subscribe_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	hustmq_ha_sub_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
	if (!ctx)
	{
		return __first_sub_handler(backend_uri, r);
	}
	if (NGX_HTTP_OK != r->headers_out.status)
	{
		ctx->subrequest_peer = ngx_http_get_next_peer(ctx->subrequest_peer);
		return ctx->subrequest_peer ? ngx_http_run_subrequest(r, &ctx->base, ctx->subrequest_peer->peer)
		        : ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
	}
	return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
}
