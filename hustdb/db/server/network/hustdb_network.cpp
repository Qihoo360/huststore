#include "hustdb_network.h"

void on_evhtp_thread_init(evhtp_t * htp, evthr_t * thr, void * arg)
{
    hustdb_network_ctx_t * ctx = reinterpret_cast<hustdb_network_ctx_t *>(arg);
    if (!ctx)
    {
        return;
    }
    ctx->append(thr);
}

void on_evhtp_thread_exit(evhtp_t * htp, evthr_t * thr, void * arg)
{

}

void hustdb_status_handler(evhtp_request_t * request, void * data)
{
    static const c_str_t reply = c_make_str("ok\n");
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
    evbase_t * evbase = event_base_new();
    if (!evbase)
    {
        return false;
    }
    evhtp_t * htp = evhtp_new(evbase, NULL);
    if (!htp)
    {
        return false;
    }

    //evhtp_set_parser_flags(htp, EVHTP_PARSE_QUERY_FLAG_LENIENT);
    evhtp_set_max_keepalive_requests(htp, ctx->max_keepalive_requests);
    evhtp_set_max_body_size(htp, ctx->max_body_size);
    evhtp_set_timeouts(htp, &ctx->recv_timeout, &ctx->send_timeout);
    if (ctx->disable_100_cont)
    {
        evhtp_disable_100_continue(htp);
    }

    htp->enable_nodelay      = ctx->enable_nodelay;
    htp->enable_defer_accept = ctx->enable_defer_accept;
    htp->enable_reuseport    = ctx->enable_reuseport;

    if (!evhtp_set_cb(htp, "/status.html", hustdb_status_handler, ctx))
    {
        return false;
    }

    if (!hustdb_init_handlers(ctx, htp))
    {
        return false;
    }

    evhtp_set_post_accept_cb(htp, on_post_accept, ctx);

    if (evhtp_use_threads_wexit(htp, on_evhtp_thread_init, on_evhtp_thread_exit, ctx->threads, ctx) < 0)
    {
        return false;
    }
    if (evhtp_bind_socket(htp, "0.0.0.0", ctx->port, ctx->backlog) < 0)
    {
        return false;
    }
    event_base_loop(evbase, 0);

    return true;
}
