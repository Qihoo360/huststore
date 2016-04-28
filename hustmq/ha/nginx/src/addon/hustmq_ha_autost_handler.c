#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_request_handler.h"
#include "hustmq_ha_fetch_stat.h"

typedef struct
{
    ngx_http_subrequest_ctx_t base;
    ngx_http_upstream_rr_peer_t * peer;
    backend_stat_array_t backend_stats;
} hustmq_ha_autost_ctx_t;

typedef struct
{
	hustmq_ha_queue_dict_t queue_dict;
	hustmq_ha_message_queue_array_t queue_array;
	hustmq_ha_buffer_t buf;
	ngx_bool_t json_encode_ok;
} hustmq_ha_stat_buffer_t;

static hustmq_ha_stat_buffer_t g_hustmq_ha_stat_buffer;

static ngx_bool_t __decode_json_array(const char * input, void * obj_val)
{
    return cjson_load_hustmqhamessagequeuearray(input, obj_val);
}

static ngx_bool_t __set_backend_stat_item(ngx_http_request_t * r, backend_stat_item_t * it)
{
    HustmqHaMessageQueueArray arr;
    if (!hustmq_ha_decode_json_array(r, __decode_json_array, &arr))
    {
        return false;
    }

    it->host = r->upstream->peer.name;
    it->arr.arr = arr.arr;
    it->arr.size = arr.size;

    return true;
}

static void __merge_backend_stats(ngx_http_hustmq_ha_main_conf_t * conf, backend_stat_array_t * backend_stats)
{
    if (backend_stats->size > 0)
    {
        hustmq_ha_update_queue_dict(conf->status_cache, backend_stats, conf->pool, &g_hustmq_ha_stat_buffer.queue_dict);
        hustmq_ha_merge_queue_dict(&g_hustmq_ha_stat_buffer.queue_dict, conf->pool,
                &g_hustmq_ha_stat_buffer.queue_array);
        g_hustmq_ha_stat_buffer.json_encode_ok = hustmqha_serialize_message_queue_array(
                &g_hustmq_ha_stat_buffer.queue_array, conf->pool, &g_hustmq_ha_stat_buffer.buf);
        hustmq_ha_dispose_backend_stat_array(backend_stats);
        hustmq_ha_invoke_evget_handler();
        hustmq_ha_invoke_evsub_handler();
    }
}

ngx_int_t hustmq_ha_init_stat_buffer(ngx_http_hustmq_ha_main_conf_t * mcf)
{
    if (NGX_OK != hustmq_ha_init_fetch_cache(mcf, __set_backend_stat_item, __merge_backend_stats))
    {
        return NGX_ERROR;
    }

    memset(&g_hustmq_ha_stat_buffer, 0, sizeof(hustmq_ha_stat_buffer_t));
	c_dict_init(&g_hustmq_ha_stat_buffer.queue_dict.dict);
	g_hustmq_ha_stat_buffer.queue_dict.queue_items = 0;

	g_hustmq_ha_stat_buffer.queue_array.size = 0;
	g_hustmq_ha_stat_buffer.queue_array.arr = ngx_palloc(mcf->pool, mcf->max_queue_size * sizeof(hustmq_ha_message_queue_item_t));

	g_hustmq_ha_stat_buffer.buf.items = mcf->max_queue_size;
	g_hustmq_ha_stat_buffer.buf.max_size = HUSTMQ_HA_QUEUE_ITEM_SIZE * g_hustmq_ha_stat_buffer.buf.items;
	g_hustmq_ha_stat_buffer.buf.buf = ngx_palloc(mcf->pool, g_hustmq_ha_stat_buffer.buf.max_size);

	g_hustmq_ha_stat_buffer.json_encode_ok = false;

	return NGX_OK;
}

hustmq_ha_queue_dict_t * hustmq_ha_get_queue_dict()
{
	return &g_hustmq_ha_stat_buffer.queue_dict;
}

ngx_int_t hustmq_ha_stat_all_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
	ngx_str_t tmp;
	tmp.data = (u_char *)g_hustmq_ha_stat_buffer.buf.buf;
	tmp.len = strlen(g_hustmq_ha_stat_buffer.buf.buf);

	return g_hustmq_ha_stat_buffer.json_encode_ok ? ngx_http_send_response_imp(
			NGX_HTTP_OK, &tmp, r) : ngx_http_send_response_imp(NGX_HTTP_NOT_FOUND, NULL, r);
}

static ngx_int_t post_autost_subrequest_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);

ngx_int_t hustmq_ha_autost_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_hustmq_ha_main_conf_t * conf = hustmq_ha_get_module_main_conf(r);
    if (!conf)
    {
        return NGX_ERROR;
    }

    hustmq_ha_autost_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
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
        hustmq_ha_autost_ctx_t * ctx = ngx_palloc(r->pool, sizeof(hustmq_ha_autost_ctx_t));
        if (!ctx)
        {
            return NGX_ERROR;
        }
        memset(ctx, 0, sizeof(hustmq_ha_autost_ctx_t));
        ngx_http_set_addon_module_ctx(r, ctx);

        ctx->backend_stats.max_size = ngx_http_get_backend_count();
        ctx->backend_stats.size = 0;
        ctx->backend_stats.arr = ngx_palloc(r->pool, ctx->backend_stats.max_size * sizeof(backend_stat_item_t));

        ctx->peer = peer;
        return ngx_http_gen_subrequest(backend_uri, r, ctx->peer,
                &ctx->base, post_autost_subrequest_handler);
    }

    ctx->peer = ngx_http_next_peer(ctx->peer);
    if (ctx->peer)
    {
        return ngx_http_run_subrequest(r, &ctx->base, ctx->peer);
    }

    r->headers_out.status = (ctx->backend_stats.size == ctx->backend_stats.max_size) ? NGX_HTTP_OK : NGX_HTTP_NOT_FOUND;

    __merge_backend_stats(conf, &ctx->backend_stats);

    return ngx_http_send_response_imp(r->headers_out.status, NULL, r);
}

static ngx_int_t post_autost_subrequest_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    r->parent->write_event_handler = ngx_http_core_run_phases;
    hustmq_ha_autost_ctx_t * ctx = data;
    if (__set_backend_stat_item(r, ctx->backend_stats.arr + ctx->backend_stats.size))
    {
        ++ctx->backend_stats.size;
    }
    return NGX_OK;
}
