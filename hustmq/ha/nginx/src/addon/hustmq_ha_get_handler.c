#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_handler_filter.h"
#include "ngx_http_upstream_check_module.h"
#include "hustmq_ha_peer_def.h"

static const ngx_str_t ACK_TOKEN = ngx_string("Ack-Token");
static const ngx_str_t ACK_PEER = ngx_string("Ack-Peer");

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
	ngx_str_t queue;
	hustmq_ha_queue_dict_t * queue_dict;
	ngx_str_t * peer_name;
	ngx_str_t * ack_token;
	ngx_bool_t ack;
} hustmq_ha_get_ctx_t;

static ngx_bool_t __check_get_queue_item(hustmq_ha_queue_item_t * item)
{
	if (!item)
	{
		return false;
	}
	json_int_t sum = 0;
	size_t i = 0;
	for (i = 0; i < HUSTMQ_HA_READY_SIZE; ++i)
	{
		sum += item->base.ready[i];
	}
	return sum > 0;
}

static ngx_bool_t __check_peer(hustmq_ha_queue_dict_t * dict, ngx_str_t * queue, ngx_http_upstream_rr_peer_t * peer)
{
    hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(dict, (const char *)queue->data);
	if (!queue_val)
	{
		return false;
	}

	hustmq_ha_queue_item_t * queue_item = hustmq_ha_host_dict_get(queue_val, (const char *)peer->name.data);
	if (!queue_item)
	{
		return false;
	}

	if(!__check_get_queue_item(queue_item))
	{
		return false;
	}
	return ngx_http_peer_is_alive(peer);
}

static ngx_http_upstream_rr_peer_t * __next_peer(hustmq_ha_queue_dict_t * dict, ngx_str_t * queue, ngx_http_upstream_rr_peer_t * peer)
{
	if (!peer)
	{
		return NULL;
	}
	if (!dict->dict.ref)
	{
		return ngx_http_next_peer(peer);
	}
	do
	{
		peer = peer->next;
		if (!peer)
		{
			return NULL;
		}
		if (__check_peer(dict, queue, peer))
		{
			return peer;
		}
	} while(peer->next);
	return NULL;
}

static ngx_http_upstream_rr_peer_t * __first_peer(hustmq_ha_queue_dict_t * dict, ngx_str_t * queue, ngx_http_upstream_rr_peer_t * peer)
{
	if (!peer)
	{
		return NULL;
	}
	if (!dict->dict.ref)
	{
		return ngx_http_first_peer(peer);
	}
	if (__check_peer(dict, queue, peer))
	{
		return peer;
	}
	return __next_peer(dict, queue, peer);
}

static ngx_int_t __post_subrequest_handler(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
    hustmq_ha_get_ctx_t * ctx = data;
    if (ctx && NGX_HTTP_OK == r->headers_out.status)
    {
        ctx->base.response.len = ngx_http_get_buf_size(&r->upstream->buffer);
        ctx->base.response.data = r->upstream->buffer.pos;
        if (!ctx->ack)
        {
            ctx->peer_name = r->upstream->peer.name;
            ctx->ack_token = ngx_http_find_head_value(&r->headers_out.headers, &ACK_TOKEN);
        }
    }
    return ngx_http_finish_subrequest(r);
}

static ngx_int_t __first_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	ngx_str_t queue = hustmq_ha_get_queue(r);
	if (!queue.data)
	{
		return NGX_ERROR;
	}

	char * val = ngx_http_get_param_val(&r->args, "ack", r->pool);
	ngx_bool_t ack = true;
	if (val && (0 == atoi(val) || 0 == strncmp(val, "false", 5)))
	{
	    ack = false;
	}

	hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();
	if (!queue_dict)
	{
		return NGX_ERROR;
	}

	if (queue_dict->dict.ref && !hustmq_ha_get_queue_item_check(queue_dict, &queue))
	{
		return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
	}
	ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
	if (!peers || !peers->peer)
	{
		return NGX_ERROR;
	}

	ngx_http_upstream_rr_peer_t * peer = __first_peer(queue_dict, &queue, peers->peer);
	if (!peer)
	{
		return NGX_ERROR;
	}

	hustmq_ha_get_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_get_ctx_t));
	if (!ctx)
	{
		return NGX_ERROR;
	}
	memset(ctx, 0, sizeof(hustmq_ha_get_ctx_t));
	ngx_http_set_addon_module_ctx(r, ctx);
	ctx->queue_dict = queue_dict;
	ctx->queue = queue;
	ctx->ack = ack;

	ctx->peer = peer;
	return ngx_http_gen_subrequest(backend_uri, r, ctx->peer,
        &ctx->base, __post_subrequest_handler);
}

static ngx_bool_t __add_response_headers(hustmq_ha_get_ctx_t * ctx, ngx_http_request_t *r)
{
    if (ctx->ack)
    {
        return true;
    }
    if (!ngx_http_add_field_to_headers_out(&ACK_TOKEN, ctx->ack_token, r))
    {
        return false;
    }
    ngx_str_t ack_peer = hustmq_ha_encode_ack_peer(ctx->peer_name, r->pool);
    if (!ngx_http_add_field_to_headers_out(&ACK_PEER, &ack_peer, r))
    {
        return false;
    }
    return true;
}

ngx_int_t hustmq_ha_get_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	hustmq_ha_get_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
	if (!ctx)
	{
		return __first_get_handler(backend_uri, r);
	}
	if (NGX_HTTP_OK != r->headers_out.status)
	{
		ctx->peer = __next_peer(ctx->queue_dict, &ctx->queue, ctx->peer);
		return ctx->peer ? ngx_http_run_subrequest(r, &ctx->base, ctx->peer) : NGX_ERROR;
	}
	if (!__add_response_headers(ctx, r))
	{
	    return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
	}
	return ngx_http_send_response_imp(NGX_HTTP_OK, &ctx->base.response, r);
}
