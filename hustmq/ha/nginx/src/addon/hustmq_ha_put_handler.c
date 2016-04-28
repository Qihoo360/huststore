#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_handler_filter.h"

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

ngx_int_t __first_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
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

ngx_int_t hustmq_ha_put_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
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
