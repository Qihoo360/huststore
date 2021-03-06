#include "hustdb_network.h"

static evbase_t * g_evbase = NULL;

void on_evhtp_thread_init(evhtp_t * htp, evthr_t * thr, void * arg)
{
    hustdb_network_ctx_t * ctx = reinterpret_cast<hustdb_network_ctx_t *>(arg);
    if (!ctx)
    {
        return;
    }
    ctx->base.append(thr);
}

void on_evhtp_thread_exit(evhtp_t * htp, evthr_t * thr, void * arg)
{

}

void hustdb_status_handler(evhtp_request_t * request, void * data)
{
    static const evhtp::c_str_t reply = evhtp_make_str("ok\n");
    evbuffer_add_reference(request->buffer_out, reply.data, reply.len, 0, 0);
    evhtp_send_reply(request, EVHTP_RES_200);
}

static evhtp_res on_post_accept(evhtp_connection_t * conn, void * data)
{
    hustdb_network::ip_allow_t * ip_allow_map = ((hustdb_network_ctx_t *) data)->ip_allow_map;
        
    if ( ip_allow_map->size > 0 && ! hustdb_network::can_access((struct sockaddr_in *)conn->saddr, ip_allow_map) )
    {
        return EVHTP_RES_ERROR;
    }
    
    return EVHTP_RES_OK;
}

bool hustdb_loop(hustdb_network_ctx_t * ctx)
{
    if (g_evbase)
    {
        return false;
    }
    g_evbase = event_base_new();
    if (!g_evbase)
    {
        return false;
    }
    evhtp_t * htp = evhtp_new(g_evbase, NULL);
    if (!htp)
    {
        return false;
    }

    //evhtp_set_parser_flags(htp, EVHTP_PARSE_QUERY_FLAG_LENIENT);
    evhtp_set_max_keepalive_requests(htp, ctx->base.max_keepalive_requests);
    evhtp_set_max_body_size(htp, ctx->base.max_body_size);
    evhtp_set_timeouts(htp, &ctx->base.recv_timeout, &ctx->base.send_timeout);
    if (ctx->base.disable_100_cont)
    {
        evhtp_disable_100_continue(htp);
    }

    htp->enable_nodelay      = ctx->base.enable_nodelay;
    htp->enable_defer_accept = ctx->base.enable_defer_accept;
    htp->enable_reuseport    = ctx->base.enable_reuseport;

    if (!evhtp_set_cb(htp, "/status.html", hustdb_status_handler, ctx))
    {
        return false;
    }

    if (!hustdb_init_handlers(ctx, htp))
    {
        return false;
    }

    evhtp_set_post_accept_cb(htp, on_post_accept, ctx);

    if (evhtp_use_threads_wexit(htp, on_evhtp_thread_init, on_evhtp_thread_exit, ctx->base.threads, ctx) < 0)
    {
        return false;
    }
    if (evhtp_bind_socket(htp, "0.0.0.0", ctx->base.port, ctx->base.backlog) < 0)
    {
        return false;
    }
    event_base_loop(g_evbase, 0);

    return true;
}

void libevhtp_exit_server()
{
    event_base_loopbreak(g_evbase);
}
