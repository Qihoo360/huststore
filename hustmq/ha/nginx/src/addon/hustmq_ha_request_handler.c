#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_utils.h"

ngx_int_t hustmq_ha_send_reply(ngx_uint_t status, ngx_buf_t * buf, size_t buf_size, ngx_http_request_t *r)
{
    r->headers_out.status = status;

    static ngx_str_t type = ngx_string("text/plain");
    r->headers_out.content_type = type;
    r->headers_out.content_length_n = buf_size;

    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;
    ngx_int_t rc =  ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
    {
        return rc;
    }

    buf->last_buf = 1;

    ngx_chain_t out;
    out.buf = buf;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}

ngx_int_t hustmq_ha_post_subrequest_handler(ngx_http_request_t * r, void * data, ngx_int_t rc)
{
	hustmq_ha_loop_ctx_t * ctx = ngx_http_get_addon_module_ctx(r->parent);
	if (ctx)
	{
		if (NGX_HTTP_OK == r->headers_out.status)
		{
			ctx->result = NGX_HTTP_OK;
		}
	}
	return ngx_http_finish_subrequest(r);
}

ngx_str_t hustmq_ha_get_queue(ngx_http_request_t * r)
{
    ngx_str_t queue = { 0, 0 };
    char * val = ngx_http_get_param_val(&r->args, "queue", r->pool);
    size_t size = strlen(val);
    if (val && hustmq_ha_check_queue(val, size))
    {
        queue.data = (u_char *)val;
        queue.len = size;
    }
    return queue;
}

int hustmq_ha_get_idx(ngx_http_request_t * r)
{
    char * val = ngx_http_get_param_val(&r->args, "idx", r->pool);
    if (!val)
    {
        return -1;
    }
    int idx = atoi(val);
    if (idx < 0)
    {
        return -1;
    }
    return idx;
}

static ngx_int_t __first_handler(ngx_str_t * backend_uri, ngx_http_request_t *r, check_request_pt checker, ngx_http_post_subrequest_pt handler)
{
	hustmq_ha_queue_dict_t * queue_dict = hustmq_ha_get_queue_dict();
	if (!queue_dict)
	{
		return NGX_ERROR;
	}

	ngx_str_t queue = hustmq_ha_get_queue(r);
	if (!queue.data)
	{
		return NGX_ERROR;
	}

	if (!checker(r, queue_dict, &queue))
	{
		return NGX_ERROR;
	}

	if (queue_dict->dict.ref)
	{
	    hustmq_ha_queue_value_t * queue_val = hustmq_ha_queue_dict_get(queue_dict, (const char *)queue.data);
		if (!queue_val)
		{
			return NGX_ERROR;
		}
	}

	ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
	if (!peers || !peers->peer)
	{
		return NGX_ERROR;
	}

	ngx_http_upstream_rr_peer_t * peer = ngx_http_first_peer(peers->peer);
	if (!peer)
	{
	    return NGX_ERROR;
	}

	hustmq_ha_loop_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_loop_ctx_t));
	if (!ctx)
	{
		return NGX_ERROR;
	}
	memset(ctx, 0, sizeof(hustmq_ha_loop_ctx_t));
	ngx_http_set_addon_module_ctx(r, ctx);

	ctx->result = NGX_HTTP_NOT_FOUND;
	ctx->peer = peer;
	return ngx_http_gen_subrequest(backend_uri, r, ctx->peer,
	        &ctx->base, handler);
}

ngx_int_t hustmq_ha_handler_base(ngx_str_t * backend_uri, ngx_http_request_t *r, check_request_pt checker, ngx_http_post_subrequest_pt handler)
{
	hustmq_ha_loop_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
	if (!ctx)
	{
		return __first_handler(backend_uri, r, checker, handler);
	}
	ctx->peer = ngx_http_next_peer(ctx->peer);
	if (ctx->peer)
	{
		return ngx_http_run_subrequest(r, &ctx->base, ctx->peer);
	}
	return NGX_HTTP_OK == ctx->result ? ngx_http_send_response_imp(NGX_HTTP_OK, NULL, r) : NGX_ERROR;
}

ngx_bool_t hustmq_ha_decode_json_array(ngx_http_request_t *r, decode_json_array_t decode, void * arr)
{
	if (NGX_HTTP_OK != r->headers_out.status)
	{
		return false;
	}

	ngx_str_t response = { ngx_http_get_buf_size(&r->upstream->buffer), r->upstream->buffer.pos };

	if (response.len < 1 || !response.data)
	{
		return false;
	}

	u_char * end = response.data + response.len;

	if (!end)
	{
		return false;
	}

	u_char end_val = *end;

	*end = '\0';
	ngx_bool_t rc = decode((const char *)response.data, arr);
	*end = end_val;

	return rc;
}
