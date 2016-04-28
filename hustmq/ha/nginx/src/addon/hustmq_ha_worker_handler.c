#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_worker_def.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
	hustmq_workers_array_t workers;
} hustmq_ha_worker_ctx_t;

typedef struct
{
	hustmq_worker_array_t worker_array;
	hustmq_ha_buffer_t workers_buf;
	hustmq_worker_dict_t worker_dict;
} hustmq_worker_buffer_t;

static hustmq_worker_buffer_t g_hustmq_worker_buffer;

void hustmq_ha_init_worker_buffer(ngx_pool_t * pool)
{
    memset(&g_hustmq_worker_buffer, 0, sizeof(hustmq_worker_buffer_t));
	c_dict_init(&g_hustmq_worker_buffer.worker_dict.dict);
	g_hustmq_worker_buffer.worker_dict.worker_items = 0;
	g_hustmq_worker_buffer.worker_array.size = 0;
	g_hustmq_worker_buffer.worker_array.arr = ngx_palloc(pool, HUSTMQ_HA_WORKER_ITEMS * sizeof(hustmq_worker_t));

	g_hustmq_worker_buffer.workers_buf.items = HUSTMQ_HA_WORKER_ITEMS;
	g_hustmq_worker_buffer.workers_buf.max_size = HUSTMQ_HA_WORKER_SIZE * g_hustmq_worker_buffer.workers_buf.items;
	g_hustmq_worker_buffer.workers_buf.buf = ngx_palloc(pool, g_hustmq_worker_buffer.workers_buf.max_size);
}

static ngx_int_t post_worker_subrequest_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);

ngx_int_t hustmq_ha_worker_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	ngx_http_hustmq_ha_main_conf_t * conf = hustmq_ha_get_module_main_conf(r);
	if (!conf)
	{
		return NGX_ERROR;
	}

	hustmq_ha_worker_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
	if (!ctx)
	{
		ngx_http_upstream_rr_peers_t * peers = ngx_http_get_backends();
		if (!peers)
		{
			return NGX_ERROR;
		}
		ngx_http_upstream_rr_peer_t * peer = ngx_http_first_peer(peers->peer);
		if (!peer)
		{
		    return NGX_ERROR;
		}
		hustmq_ha_worker_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_worker_ctx_t));
		if (!ctx)
		{
			return NGX_ERROR;
		}
		memset(ctx, 0, sizeof(hustmq_ha_worker_ctx_t));
		ngx_http_set_addon_module_ctx(r, ctx);

		ctx->workers.max_size = ngx_http_get_backend_count();
		ctx->workers.size = 0;
		ctx->workers.arr = ngx_palloc(r->pool, ctx->workers.max_size * sizeof(HustmqWorkerArray));

		ctx->peer = peer;
		return ngx_http_gen_subrequest(backend_uri, r, ctx->peer,
		        &ctx->base, post_worker_subrequest_handler);
	}

	ctx->peer = ngx_http_next_peer(ctx->peer);
	if (ctx->peer)
	{
		return ngx_http_run_subrequest(r, &ctx->base, ctx->peer);
	}

	r->headers_out.status = (ctx->workers.size > 0) ? NGX_HTTP_OK : NGX_HTTP_NOT_FOUND;

	if (ctx->workers.size > 0)
	{
		hustmq_ha_update_worker_dict(&ctx->workers, conf->pool, &g_hustmq_worker_buffer.worker_dict);
		hustmq_ha_merge_worker_dict(&g_hustmq_worker_buffer.worker_dict, conf->pool, &g_hustmq_worker_buffer.worker_array);
		ngx_bool_t rc = hustmqha_serialize_worker_array(&g_hustmq_worker_buffer.worker_array, conf->pool, &g_hustmq_worker_buffer.workers_buf);
		hustmq_ha_dispose_hustmq_workers_array(&ctx->workers);

		if (rc)
		{
			ngx_str_t tmp;
			tmp.data = (u_char *)g_hustmq_worker_buffer.workers_buf.buf;
			tmp.len = strlen(g_hustmq_worker_buffer.workers_buf.buf);

			return ngx_http_send_response_imp(NGX_HTTP_OK, &tmp, r);
		}
	}
	return ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
}

static ngx_bool_t __decode_json_array(const char * input, void * obj_val)
{
	return cjson_load_hustmqworkerarray(input, obj_val);
}

static ngx_int_t post_worker_subrequest_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
	r->parent->write_event_handler = ngx_http_core_run_phases;

	HustmqWorkerArray arr;
	if (!hustmq_ha_decode_json_array(r, __decode_json_array, &arr))
	{
		return NGX_OK;
	}

	hustmq_ha_worker_ctx_t * ctx = data;

	HustmqWorkerArray * it = ctx->workers.arr + ctx->workers.size;
	it->arr = arr.arr;
	it->size = arr.size;
	++ctx->workers.size;

	return NGX_OK;
}
