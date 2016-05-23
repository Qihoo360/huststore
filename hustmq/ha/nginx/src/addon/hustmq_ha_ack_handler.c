#include "hustmq_ha_handler.h"
#include "hustmq_ha_utils.h"
#include "hustmq_ha_peer_def.h"
#include "hustmq_ha_request_handler.h"

ngx_int_t hustmq_ha_ack_handler(ngx_str_t * backend_uri, ngx_http_request_t *r)
{
    ngx_http_subrequest_ctx_t * ctx = ngx_http_get_addon_module_ctx(r);
    if (!ctx)
    {
        static ngx_str_t PEER = ngx_string("peer");

        char * val = ngx_http_get_param_val(&r->args, (const char *)PEER.data, r->pool);
        if (!val)
        {
            return NGX_ERROR;
        }
        ngx_str_t ack_peer = { strlen(val), (u_char *)val };
        ngx_http_upstream_rr_peer_t * peer = hustmq_ha_decode_ack_peer(&ack_peer, r->pool);
        if (!peer)
        {
            return NGX_ERROR;
        }

        ngx_http_subrequest_ctx_t * ctx = ngx_palloc(r->pool, sizeof(ngx_http_subrequest_ctx_t));
        if (!ctx)
        {
            return NGX_ERROR;
        }
        memset(ctx, 0, sizeof(ngx_http_subrequest_ctx_t));
        ngx_http_set_addon_module_ctx(r, ctx);

        ctx->args = ngx_http_remove_param(&r->args, &PEER, r->pool);

        return ngx_http_gen_subrequest(backend_uri, r, peer,
            ctx, ngx_http_post_subrequest_handler);
    }
    return ngx_http_send_response_imp(r->headers_out.status, NULL, r);
}
